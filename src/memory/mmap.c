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
#include <kernel/memory.h>
#include <kernel/file.h>
#include <kernel/sched.h>
#include <kernel/servers.h>

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
    if(!p) return (void *) -ESRCH;

    IODescriptor *io = &p->io[fd];
    if(!io->valid || !io->data) return (void *) -EBADF;
    if(io->type != IO_FILE) return (void *) -ENODEV;

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

    int status = requestServer(t, command);
    free(command);
    return (void *) (intptr_t) status;
}