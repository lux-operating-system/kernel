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

    for(;;) schedule();  // force context switch!
}

/* syscallEnqueue(): enqueues a syscall request
 * params: request - pointer to the request
 * returns: pointer to the request
 */

SyscallRequest *syscallEnqueue(SyscallRequest *request) {
    schedLock();

    if(!requests) {
        requests = request;
    } else {
        SyscallRequest *q = request;
        while(q->next) {
            q = q->next;
        }

        q->next = request;
    }

    schedRelease();
    return request;
}

/* syscallDequeue(): dequeues a syscall request
 * params: none
 * returns: pointer to the request, NULL if queue is empty
 */

SyscallRequest *syscallDequeue() {
    if(!requests) return NULL;

    schedLock();
    SyscallRequest *request = requests;
    requests = requests->next;

    request->busy = true;   // prevent multiple CPUs accessing the same request
    schedRelease();
    return request;
}
