/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

#include <stdlib.h>
#include <platform/platform.h>
#include <platform/context.h>
#include <kernel/sched.h>
#include <kernel/logger.h>

/* fork(): forks the running thread
 * params: t - pointer to thread structure
 * returns: zero to child, PID of child to parent, negative on fail
 */

pid_t fork(Thread *t) {
    schedLock();
    setScheduling(false);

    pid_t pid = processCreate();
    if(!pid) {
        setScheduling(true);
        schedRelease();
        return -1;
    }

    //KDEBUG("forking parent PID %d into child %d\n", t->pid, pid);

    // we now have a blank slate process, so we need to create a thread in it
    // and deep clone the parent into the child
    Process *p = getProcess(pid);
    p->threadCount = 1;
    p->threads = calloc(p->threadCount, sizeof(Thread *));
    if(!p->threads) {
        free(p);
        setScheduling(true);
        schedRelease();
        return -1;
    }

    p->threads[0] = calloc(1, sizeof(Thread));
    if(!p->threads[0]) {
        free(p->threads);
        free(p);
        setScheduling(true);
        schedRelease();
        return -1;
    }

    // add the new thread to the queue
    p->threads[0]->status = THREAD_QUEUED;
    p->threads[0]->next = NULL;
    p->threads[0]->pid = pid;
    p->threads[0]->tid = pid;
    p->threads[0]->context = calloc(1, PLATFORM_CONTEXT_SIZE);
    if(!p->threads[0]->context) {
        free(p->threads[0]);
        free(p->threads);
        free(p);
        setScheduling(true);
        schedRelease();
        return -1;
    }

    // and clone the parent's context
    if(!platformCloneContext(p->threads[0]->context, t->context)) {
        free(p->threads[0]->context);
        free(p->threads[0]);
        free(p->threads);
        free(p);
        setScheduling(true);
        schedRelease();
        return -1;
    }

    processes++;
    threads++;
    schedAdjustTimeslice();

    // and we're done - return zero to the child
    platformSetContextStatus(p->threads[0]->context, 0);
    setScheduling(true);
    schedRelease();
    return pid;     // and PID to the parent
}
