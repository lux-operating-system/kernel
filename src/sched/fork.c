/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

#include <stdlib.h>
#include <string.h>
#include <platform/platform.h>
#include <platform/context.h>
#include <kernel/sched.h>
#include <kernel/logger.h>
#include <kernel/signal.h>

/* fork(): forks the running thread
 * params: t - pointer to thread structure
 * returns: zero to child, PID of child to parent, negative on fail
 */

pid_t fork(Thread *t) {
    schedLock();

    pid_t pid = processCreate();
    if(!pid) {
        schedRelease();
        return -1;
    }

    // we now have a blank slate process, so we need to create a thread in it
    // and deep clone the parent into the child
    Process *p = getProcess(pid);
    p->parent = t->pid;     // NOTICE: not sure if we should be using the PID or TID of the parent
    p->threadCount = 1;
    p->threads = calloc(p->threadCount, sizeof(Thread *));
    if(!p->threads) {
        free(p);
        schedRelease();
        return -1;
    }

    p->threads[0] = calloc(1, sizeof(Thread));
    if(!p->threads[0]) {
        free(p->threads);
        free(p);
        schedRelease();
        return -1;
    }

    // add the new thread to the queue
    p->threads[0]->status = THREAD_QUEUED;
    p->threads[0]->next = NULL;
    p->threads[0]->pid = pid;
    p->threads[0]->tid = pid;
    p->threads[0]->context = calloc(1, PLATFORM_CONTEXT_SIZE);
    p->threads[0]->signalContext = calloc(1, PLATFORM_CONTEXT_SIZE);
    p->threads[0]->highest = t->highest;
    p->threads[0]->pages = t->pages;

    // NOTE: fork() only clones one thread, which is why we're not cloning the
    // entire process memory, but just the calling thread
    p->pages = t->pages;

    if(!p->threads[0]->context || !p->threads[0]->signalContext) {
        free(p->threads[0]);
        free(p->threads);
        free(p);
        schedRelease();
        return -1;
    }

    // and clone the parent's context
    if(!platformCloneContext(p->threads[0]->context, t->context)) {
        free(p->threads[0]->context);
        free(p->threads[0]);
        free(p->threads);
        free(p);
        schedRelease();
        return -1;
    }

    // clone signal handlers
    p->threads[0]->signals = signalClone(t->signals);

    // clone I/O descriptors
    Process *parent = getProcess(t->pid);
    if(parent) {
        memcpy(p->io, parent->io, sizeof(IODescriptor) * MAX_IO_DESCRIPTORS);
        p->iodCount = parent->iodCount;

        for(int i = 0; i < MAX_IO_DESCRIPTORS; i++)
            if(p->io[i].valid) p->io[i].clone = true;

        // clone working directory
        strcpy(p->cwd, parent->cwd);

        // clone command line and process name
        strcpy(p->name, parent->name);
        strcpy(p->command, parent->command);

        // if we made this far then the creation was successful
        // list the child process as a child of the parent
        parent->childrenCount++;
        parent->children = realloc(parent->children, parent->childrenCount);
        if(parent->children) parent->children[parent->childrenCount-1] = p;
    }

    processes++;
    threads++;
    schedAdjustTimeslice();

    // and we're done - return zero to the child
    platformSetContextStatus(p->threads[0]->context, 0);
    schedRelease();
    return pid;     // and PID to the parent
}
