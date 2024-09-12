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

    return requestServer(t, command);
}
