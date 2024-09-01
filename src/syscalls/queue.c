/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

#include <platform/platform.h>
#include <kernel/syscalls.h>
#include <kernel/sched.h>

static SyscallRequest *requests = NULL;    // sort of a linked list in a sense

/* syscallHandle(): generic handler for system calls
 * params: ctx - context of the current thread
 * returns: nothing
 */

void syscallHandle(void *ctx) {
    Thread *t = getThread(getTid());
    if(t) {
        platformSaveContext(t->context, ctx);
        syscallEnqueue(platformCreateSyscallContext(t));    // queue the request
        blockThread(t);     // block until we handle the syscall
    }
}