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

void handleSyscallResponse(const SyscallHeader *hdr) {
    SyscallRequest *req = getSyscall(hdr->header.requester);
    if(!req) {
        KWARN("received response for syscall 0x%X pid %d but no such request exists\n", hdr->header.command, hdr->header.requester);
        return;
    }

    if(req->requestID != hdr->id) {
        KWARN("received response for syscall 0x%X (kernel syscall %d) pid %d but IDs mismatch; terminating thread\n", hdr->header.command, req->function, hdr->header.requester);
        schedLock();
        terminateThread(req->thread, -1, false);
        schedRelease();
        return;
    }

    // default action is to unblock the thread
    schedLock();
    Process *p = getProcess(req->thread->pid);
    req->ret = hdr->header.status;
    req->external = false;
    req->unblock = true;
    req->thread->time = schedTimeslice(req->thread, req->thread->priority);
    req->thread->status = THREAD_QUEUED;

    // some syscalls will require special handling
    ssize_t status;
    FileDescriptor *file;

    switch(hdr->header.command) {
    case COMMAND_STAT:
        if(hdr->header.status) break;
        StatCommand *statcmd = (StatCommand *) hdr;
        threadUseContext(req->thread->tid);
        memcpy((void *)req->params[1], &statcmd->buffer, sizeof(struct stat));
        break;

    case COMMAND_OPEN:
        if(hdr->header.status) break;
        OpenCommand *opencmd = (OpenCommand *) hdr;

        // set up a file descriptor for the process
        IODescriptor *iod = NULL;
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
            schedRelease();
            syscallEnqueue(req);
            break;
        } else if(status < 0) break;  // here an actual error happened
        
        RWCommand *readcmd = (RWCommand *) hdr;
        threadUseContext(req->thread->tid);
        memcpy((void *)req->params[1], readcmd->data, hdr->header.status);

        // update file position
        file = (FileDescriptor *) p->io[req->params[0]].data;
        file->position = readcmd->position;

        break;
    }

    platformSetContextStatus(req->thread->context, req->ret);
    schedRelease();
}
