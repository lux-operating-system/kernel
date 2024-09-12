/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

#include <platform/platform.h>
#include <platform/context.h>
#include <kernel/syscalls.h>
#include <kernel/sched.h>
#include <kernel/logger.h>

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

    request->queued = true;

    if(!requests) {
        requests = request;
    } else {
        SyscallRequest *q = requests;
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
    schedLock();
    if(!requests) {
        schedRelease();
        return NULL;
    }

    SyscallRequest *request = requests;
    requests = requests->next;
    request->busy = true;
    request->queued = false;

    schedRelease();
    return request;
}

/* syscallProcess(): processes syscalls in the queue from the kernel threads
 * params: none
 * returns: zero if syscall queue is empty
 */

int syscallProcess() {
    SyscallRequest *syscall = syscallDequeue();
    if(!syscall) return 0;

    // essentially just dispatch the syscall and store the return value
    // in the thread's context so it can get it back
    if(syscall->function > MAX_SYSCALL || !syscallDispatchTable[syscall->function]) {
        KWARN("undefined syscall request %d from tid %d, killing thread...\n", syscall->function, syscall->thread->tid);
        schedLock();
        terminateThread(syscall->thread, -1, false);
        schedRelease();
    } else {
        threadUseContext(syscall->thread->tid);
        syscallDispatchTable[syscall->function](syscall);
        platformSetContextStatus(syscall->thread->context, syscall->ret);
        //threadUseContext(getTid());
    }

    if((syscall->thread->status == THREAD_BLOCKED) && syscall->unblock) {
        // this way we prevent accidentally running threads that exit()
        syscall->thread->status = THREAD_QUEUED;
        syscall->thread->time = schedTimeslice(syscall->thread, syscall->thread->priority);
        syscall->busy = false;
    }

    return 1;
}

/* getSyscall(): returns the syscall request structure of a thread
 * params: tid - thread ID
 * returns: pointer to syscall structure, NULL on fail
 */

SyscallRequest *getSyscall(pid_t tid) {
    Thread *t = getThread(tid);
    if(!t) return NULL;

    return &t->syscall;
}
