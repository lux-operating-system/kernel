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

    MmapHeader *header = (MmapHeader *) base;
    header->fd = p->fd;
    header->flags = p->flags;
    header->length = msg->len;
    header->offset = msg->off;
    header->prot = msg->prot;
    header->pid = req->thread->pid;
    header->tid = req->thread->tid;

    // mmap adds one extra reference to a file descriptor
    // so it will not be closed even when close() is invoked
    Process *proc = getProcess(req->thread->pid);
    FileDescriptor *file = (FileDescriptor *) proc->io[p->fd].data;
    file->refCount++;

    if(msg->responseType) {
        /* memory-mapped device file */
        header->device = true;
        base += PAGE_SIZE;      // skip over to the next page

        for(size_t i = 0; i < pageCount; i++)
            platformMapPage(base + (i*PAGE_SIZE), msg->mmio + (i*PAGE_SIZE), pageFlags);

        req->ret = base;
    } else {
        /* memory-mapped regular file */
        header->device = false;
        base += PAGE_SIZE;

        memcpy((void *) base, msg->data, msg->len);
        memset((void *) (uintptr_t) base + msg->len, 0, PAGE_SIZE - msg->len);
        req->ret = base;
    }
}