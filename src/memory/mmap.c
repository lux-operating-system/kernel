/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <kernel/logger.h>
#include <kernel/memory.h>
#include <kernel/file.h>
#include <kernel/sched.h>
#include <kernel/servers.h>
#include <platform/platform.h>

/* mmap(): creates a memory mapping for a file descriptor
 * params: t - calling thread
 * params: id - syscall ID
 * params: addr - process-suggested address
 * params: len - length of the mapping
 * params: prot - protection flags
 * params: flags - mapping flags
 * params: fd - file descriptor
 * params: off - offset into file descriptor
 * returns: pointer to the memory mapping, negative error code on fail
 */

void *mmap(Thread *t, uint64_t id, void *addr, size_t len, int prot, int flags,
           int fd, off_t off) {
    if(flags & MAP_FIXED) {
        uintptr_t base = (uintptr_t) addr;
        if(base & (PAGE_SIZE-1)) return (void *) -EINVAL;
        base -= PAGE_SIZE;
        uintptr_t end = base + len;
        if((base < t->highest) || (end >= USER_LIMIT_ADDRESS))
            return (void *) -ENOMEM;
    }

    if(flags & MAP_ANONYMOUS) {
        size_t pageCount = (len+PAGE_SIZE-1) / PAGE_SIZE;
        int pageFlags = PLATFORM_PAGE_PRESENT | PLATFORM_PAGE_USER;
        if(prot & PROT_WRITE) pageFlags |= PLATFORM_PAGE_WRITE;
        if(prot & PROT_EXEC) pageFlags |= PLATFORM_PAGE_EXEC;

        uintptr_t anon;
        if(flags & MAP_FIXED) {
            anon = vmmAllocate(USER_MMIO_BASE, USER_LIMIT_ADDRESS, pageCount+1, pageFlags);
        } else {
            uintptr_t base = (uintptr_t) addr;

            anon = vmmAllocate(base, USER_LIMIT_ADDRESS, pageCount+1, pageFlags);
            if(anon && (anon != base)) {
                vmmFree(anon, pageCount+1);
                return (void *) -ENOMEM;
            }
        }
    
        if(!anon) return (void *) -ENOMEM;

        memset((void *) anon, 0, (pageCount+1) * PAGE_SIZE);
        MmapHeader *hdr = (MmapHeader *) anon;
        hdr->flags = flags;
        hdr->length = len;
        hdr->pid = t->pid;
        hdr->tid = t->tid;
        hdr->fd = -1;

        return (void *) ((uintptr_t) anon + PAGE_SIZE);
    }

    if(fd < 0 || fd > MAX_IO_DESCRIPTORS) return (void *) -EBADF;
    MmapCommand *command = calloc(1, sizeof(MmapCommand));
    if(!command) return (void *) -ENOMEM;

    Process *p = getProcess(t->pid);
    if(!p) {
        free(command);
        return (void *) -ESRCH;
    }

    IODescriptor *io = &p->io[fd];
    if(!io->valid || !io->data) {
        free(command);
        return (void *) -EBADF;
    }

    if(io->type != IO_FILE) {
        free(command);
        return (void *) -ENODEV;
    }

    FileDescriptor *f = (FileDescriptor *) io->data;

    command->header.header.command = COMMAND_MMAP;
    command->header.header.length = sizeof(MmapCommand);
    command->header.id = id;
    command->uid = p->user;
    command->gid = p->group;
    command->position = f->position;
    command->openFlags = io->flags;
    command->id = f->id;
    strcpy(command->device, f->device);
    strcpy(command->path, f->abspath);

    command->addr = addr;
    command->len = len;
    command->prot = prot;
    command->flags = flags;
    command->off = off;

    int status = requestServer(t, 0, command);
    free(command);
    return (void *) (intptr_t) status;
}

/* mmapHandle(): handler for mmap() response message from a driver
 * params: msg - response message
 * params: req - relevant system call request
 * returns: nothing
 */

void mmapHandle(MmapCommand *msg, SyscallRequest *req) {
    struct MmapSyscallParams *p = (struct MmapSyscallParams *) req->params[0];

    size_t pageCount = (msg->len+PAGE_SIZE-1) / PAGE_SIZE;
    int pageFlags = PLATFORM_PAGE_PRESENT | PLATFORM_PAGE_USER;
    if(msg->prot & PROT_WRITE) pageFlags |= PLATFORM_PAGE_WRITE;
    if(msg->prot & PROT_EXEC) pageFlags |= PLATFORM_PAGE_EXEC;

    // allocate one extra page for the mmap() header so we can unmap later
    // the difference between this and malloc() is that this must always be
    // page-aligned to comply with POSIX
    uintptr_t base = vmmAllocate(USER_MMIO_BASE, USER_LIMIT_ADDRESS, pageCount+1, VMM_USER | VMM_WRITE);
    if(!base) {
        req->ret = -ENOMEM;
        return;
    }

    // first page will be reserved for the mapping
    MmapHeader *header = (MmapHeader *) base;
    header->fd = p->fd;
    header->flags = p->flags;
    header->length = msg->len;
    header->offset = msg->off;
    header->prot = msg->prot;
    header->pid = req->thread->pid;
    header->tid = req->thread->tid;

    base += PAGE_SIZE;      // skip over to the next page

    // mmap adds one extra reference to a file descriptor
    // so it will not be closed even when close() is invoked
    Process *proc = getProcess(req->thread->pid);
    FileDescriptor *file = (FileDescriptor *) proc->io[p->fd].data;
    file->refCount++;

    if(msg->responseType) {
        /* memory-mapped device file */
        header->device = true;

        for(size_t i = 0; i < pageCount; i++)
            platformMapPage(base + (i*PAGE_SIZE), msg->mmio + (i*PAGE_SIZE), pageFlags);

        req->ret = base;
    } else {
        /* memory-mapped regular file */
        header->device = false;

        memcpy((void *) base, msg->data, msg->len);
        memset((void *)((uintptr_t) base + msg->len), 0, PAGE_SIZE - msg->len);
        req->ret = base;
    }
}

/* munmap(): unmaps a memory-mapped file
 * params: t - calling thread
 * params: addr - address of the mapping
 * params: len - length to unmap
 * returns: zero on success, negative errno on fail
 */

int munmap(Thread *t, void *addr, size_t len) {
    uintptr_t ptr = (uintptr_t) addr;
    if(ptr & (PAGE_SIZE-1)) return -EINVAL;
    if(ptr < USER_MMIO_BASE || ptr > USER_LIMIT_ADDRESS) return -EINVAL;
    if(!len) return -EINVAL;

    MmapHeader *header = (MmapHeader *)((uintptr_t) ptr-PAGE_SIZE);
    if(len > header->length) return -EINVAL;
    int fd = header->fd;    // back this up before unmapping

    if(fd > 0 && fd <= MAX_IO_DESCRIPTORS) {
        Process *p = getProcess(t->pid);
        if(!p) return -ESRCH;
        if(!p->io[fd].valid || (p->io[fd].type != IO_FILE))
            return -EINVAL;
        
        FileDescriptor *file = (FileDescriptor *) p->io[header->fd].data;
        if(!file) return -EINVAL;

        file->refCount--;
        if(!file->refCount) {
            free(file);
            closeIO(p, &p->io[fd]);
        }
    }

    size_t pageCount = (len+PAGE_SIZE)/PAGE_SIZE;

    if(header->device) {
        vmmFree(ptr-PAGE_SIZE, 1);
        for(int i = 0; i < pageCount; i++) platformUnmapPage(ptr + (i*PAGE_SIZE));
    } else {
        vmmFree(ptr-PAGE_SIZE, pageCount+1);
    }

    return 0;
}

/* msync(): syncs disk storage with memory-mapped I/O
 * params: t - calling thread
 * params: addr - address of the mapping
 * params: len - length to be synced
 * params: flags - synchronous/asynchronous toggle
 * returns: 0 on success, 1 if nothing to be done, negative errno on fail
 */

int msync(Thread *t, uint64_t id, void *addr, size_t len, int flags) {
    uintptr_t ptr = (uintptr_t) addr;
    if(ptr & (PAGE_SIZE-1)) return -EINVAL;
    if(ptr < USER_MMIO_BASE || ptr > USER_LIMIT_ADDRESS) return -EINVAL;
    if(!len) return -EINVAL;

    MmapHeader *header = (MmapHeader *)((uintptr_t) ptr-PAGE_SIZE);
    if(header->fd < 0) return -EINVAL;

    if(len > header->length) return -EINVAL;

    if(header->device) return 1;    // nothing to do for physical device mmio
    if(header->flags & MAP_PRIVATE) return 1;
    if(!(header->prot & PROT_WRITE)) return 1;

    Process *p = getProcess(t->pid);
    if(!p) return -ESRCH;
    if(!p->io[header->fd].valid || (p->io[header->fd].type != IO_FILE))
        return -EINVAL;

    FileDescriptor *file = (FileDescriptor *) p->io[header->fd].data;
    if(!file) return -EINVAL;

    MsyncCommand *cmd = calloc(1, sizeof(MsyncCommand) + len);
    if(!cmd) return -ENOMEM;

    cmd->header.header.command = COMMAND_MSYNC;
    cmd->header.header.length = sizeof(MsyncCommand) + len;
    cmd->header.id = id;
    cmd->uid = p->user;
    cmd->gid = p->group;
    cmd->mapFlags = header->flags;
    cmd->syncFlags = flags;
    cmd->off = header->offset;

    cmd->id = file->id;
    strcpy(cmd->path, file->path);
    strcpy(cmd->device, file->device);
    memcpy(cmd->data, addr, len);

    int status = requestServer(t, file->sd, cmd);
    free(cmd);
    return status;
}
