/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

/* Wrappers for file I/O functions */
/* None of these functions actually do anything because the microkernel has no
 * concept of files; these functions just relay the calls to lumen and request
 * a user space server to fulfill their requests */

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <kernel/file.h>
#include <kernel/io.h>
#include <kernel/sched.h>
#include <kernel/servers.h>
#include <sys/types.h>
#include <sys/stat.h>

int mount(Thread *t, uint64_t id, const char *src, const char *tgt, const char *type, int flags) {
    // send a request to lumen
    MountCommand *command = calloc(1, sizeof(MountCommand));
    if(!command) return -ENOMEM;

    command->header.header.command = COMMAND_MOUNT;
    command->header.header.length = sizeof(MountCommand);
    command->header.id = id;
    command->flags = flags;
    strcpy(command->source, src);
    strcpy(command->target, tgt);
    strcpy(command->type, type);

    int status = requestServer(t, command);
    free(command);
    return status;
}

int stat(Thread *t, uint64_t id, const char *path, struct stat *buffer) {
    StatCommand *command = calloc(1, sizeof(StatCommand));
    if(!command) return -ENOMEM;

    command->header.header.command = COMMAND_STAT;
    command->header.header.length = sizeof(StatCommand);
    command->header.id = id;
    strcpy(command->path, path);

    int status = requestServer(t, command);
    free(command);
    return status;
}

int open(Thread *t, uint64_t id, const char *path, int flags, mode_t mode) {
    OpenCommand *command = calloc(1, sizeof(OpenCommand));
    if(!command) return -ENOMEM;

    Process *p = getProcess(t->pid);
    if(!p) return -ESRCH;

    command->header.header.command = COMMAND_OPEN;
    command->header.header.length = sizeof(OpenCommand);
    command->header.id = id;
    command->flags = flags;
    command->mode = mode;
    command->uid = p->user;
    command->gid = p->group;
    strcpy(command->abspath, path);

    int status = requestServer(t, command);
    free(command);
    return status;
}