/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

/* Kernel-Server Communication */

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <platform/platform.h>
#include <kernel/servers.h>
#include <kernel/sched.h>
#include <kernel/syscalls.h>
#include <kernel/logger.h>
#include <kernel/io.h>
#include <kernel/memory.h>

void handleSyscallResponse(int sd, const SyscallHeader *hdr) {
    SyscallRequest *req = getSyscall(hdr->header.requester);
    if(!req || !req->external || req->thread->status != THREAD_BLOCKED)
        return;

    // default action is to unblock the thread
    Process *p = getProcess(req->thread->pid);
    req->ret = hdr->header.status;
    req->external = false;
    req->unblock = true;

    // some syscalls will require special handling
    ssize_t status;
    IODescriptor *iod;
    FileDescriptor *file;
    DirectoryDescriptor *dir;
    int dd;

    switch(hdr->header.command) {
    case COMMAND_STAT:
        if(hdr->header.status) break;
        StatCommand *statcmd = (StatCommand *) hdr;
        threadUseContext(req->thread->tid);
        memcpy((void *)req->params[1], &statcmd->buffer, sizeof(struct stat));
        break;
    
    case COMMAND_STATVFS:
        if(hdr->header.status) break;
        StatvfsCommand *statvfscmd = (StatvfsCommand *) hdr;
        threadUseContext(req->thread->tid);
        memcpy((void *)req->params[1], &statvfscmd->buffer, sizeof(struct statvfs));
        break;

    case COMMAND_OPEN:
        if(hdr->header.status) break;
        OpenCommand *opencmd = (OpenCommand *) hdr;

        // set up a file descriptor for the process
        iod = NULL;
        int fd = openIO(p, (void **) &iod);
        if(fd < 0 || !iod) req->ret = fd;

        iod->type = IO_FILE;
        iod->flags = opencmd->flags;
        iod->data = calloc(1, sizeof(FileDescriptor));
        if(!iod->data) {
            closeIO(p, iod);
            req->ret = -ENOMEM;
            break;
        }

        file = (FileDescriptor *) iod->data;
        file->process = p;
        file->id = opencmd->id;     // unique ID for device files
        file->refCount = 1;
        file->sd = sd;
        file->charDev = opencmd->charDev;

        strcpy(file->abspath, opencmd->abspath);
        strcpy(file->device, opencmd->device);
        strcpy(file->path, opencmd->path);

        // and return the file descriptor to the thread
        req->ret = fd;
        break;

    case COMMAND_READ:
        status = (ssize_t) hdr->header.status;

        if((status == -EWOULDBLOCK || status == -EAGAIN) && !(p->io[req->params[0]].flags & O_NONBLOCK)) {
            // continue blocking the thread if necessary
            req->thread->status = THREAD_BLOCKED;
            req->unblock = false;
            req->busy = false;
            req->queued = true;
            req->next = NULL;
            req->retry = true;
            syscallEnqueue(req);
            return;
        } else if(status < 0) break;  // here an actual error happened
        
        RWCommand *readcmd = (RWCommand *) hdr;
        threadUseContext(req->thread->tid);
        memcpy((void *)req->params[1], readcmd->data, hdr->header.status);

        // update file position
        file = (FileDescriptor *) p->io[req->params[0]].data;
        file->position = readcmd->position;

        break;

    case COMMAND_WRITE:
        status = (ssize_t) hdr->header.status;

        if((status == -EWOULDBLOCK || status == -EAGAIN) && !(p->io[req->params[0]].flags & O_NONBLOCK)) {
            // continue blocking the thread if necessary
            req->thread->status = THREAD_BLOCKED;
            req->unblock = false;
            req->busy = false;
            req->queued = true;
            req->next = NULL;
            req->retry = true;
            syscallEnqueue(req);
            return;
        } else if(status < 0) break;  // here an actual error happened

        RWCommand *writecmd = (RWCommand *) hdr;

        // update file position
        file = (FileDescriptor *) p->io[req->params[0]].data;
        file->position = writecmd->position;
        break;

    case COMMAND_IOCTL:
        // some ioctl() opcodes require us to write into an output buffer, so
        // check for those but only if the command succeeded
        IOCTLCommand *ioctlcmd = (IOCTLCommand *) hdr;
        status = (ssize_t) hdr->header.status;

        if((status >= 0) && (ioctlcmd->opcode & IOCTL_OUT_PARAM)) {
            threadUseContext(req->thread->tid);
            unsigned long *out = (unsigned long *) req->params[2];
            *out = ioctlcmd->parameter;
        }

        break;
    
    case COMMAND_OPENDIR:
        if(hdr->header.status) break;
        OpendirCommand *opendircmd = (OpendirCommand *) hdr;

        // set up a file descriptor for the process
        iod = NULL;
        dd = openIO(p, (void **) &iod);
        if(dd < 0 || !iod) req->ret = dd;

        iod->type = IO_DIRECTORY;
        iod->data = calloc(1, sizeof(DirectoryDescriptor));
        if(!iod->data) {
            closeIO(p, iod);
            req->ret = -ENOMEM;
            break;
        }

        dir = (DirectoryDescriptor *) iod->data;
        dir->process = p;
        strcpy(dir->path, opendircmd->abspath);
        strcpy(dir->device, opendircmd->device);

        // and return the directory descriptor to the thread
        req->ret = dd | DIRECTORY_DESCRIPTOR_FLAG;
        break;
    
    case COMMAND_READDIR:
        if(hdr->header.status) break;
        ReaddirCommand *readdircmd = (ReaddirCommand *) hdr;

        // update the directory descriptor's internal position
        dd = (int) req->params[0] & ~(DIRECTORY_DESCRIPTOR_FLAG);
        dir = (DirectoryDescriptor *) p->io[dd].data;
        dir->position = readdircmd->position;

        // and copy the descriptor and write its pointer into the buffer
        threadUseContext(req->thread->tid);
        struct dirent **direntptr = (struct dirent **) req->params[2];
        if(!readdircmd->end) {
            memcpy((void *)req->params[1], &readdircmd->entry, sizeof(struct dirent) + strlen(readdircmd->entry.d_name) + 1);
            *direntptr = (struct dirent *) req->params[1];
        } else {
            *direntptr = NULL;
        }

        break;
    
    case COMMAND_EXEC:
        if(hdr->header.status) break;

        schedLock();

        int execStatus = execveHandle((ExecCommand *) hdr);

        if(!execStatus) {
            // current process has been replaced
            schedRelease();
            return;
        } else {
            req->ret = execStatus;
        }

        break;
    
    case COMMAND_CHDIR:
        if(hdr->header.status) break;

        ChdirCommand *chdircmd = (ChdirCommand *) hdr;
        strcpy(p->cwd, chdircmd->path);
        break;
    
    case COMMAND_MMAP:
        if(hdr->header.status) break;

        MmapCommand *mmapcmd = (MmapCommand *) hdr;
        threadUseContext(req->thread->tid);
        mmapHandle(mmapcmd, req);
        break;

    case COMMAND_READLINK:
        if(hdr->header.status <= 0) break;

        ReadLinkCommand *rlcmd = (ReadLinkCommand *) hdr;
        threadUseContext(req->thread->tid);

        size_t linkLength = hdr->header.status;
        if(linkLength > req->params[2]) linkLength = req->params[2];

        req->ret = linkLength;
        memcpy((void *) req->params[1], rlcmd->path, linkLength);       
        break;

    case COMMAND_FSYNC:
        if(hdr->header.status) break;

        /* special handling for close() after syncing I/O */
        FsyncCommand *fscmd = (FsyncCommand *) hdr;
        if(!fscmd->close) break;

        file = (FileDescriptor *) p->io[req->params[0]].data;
        if(!file) break;

        file->refCount--;
        if(!file->refCount) free(file);
        closeIO(p, &p->io[req->params[0]]);
    }

    platformSetContextStatus(req->thread->context, req->ret);
    req->thread->status = THREAD_QUEUED;
}
