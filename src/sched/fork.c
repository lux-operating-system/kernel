/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <platform/platform.h>
#include <platform/context.h>
#include <kernel/sched.h>
#include <kernel/logger.h>
#include <kernel/signal.h>
#include <kernel/socket.h>

/* fork(): forks the running thread
 * params: t - pointer to thread structure
 * returns: zero to child, PID of child to parent, negative on fail
 */

pid_t fork(Thread *t) {
    schedLock();

    pid_t pid = processCreate();
    if(!pid) {
        schedRelease();
        return -EAGAIN;
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
        return -ENOMEM;
    }

    p->threads[0] = calloc(1, sizeof(Thread));
    if(!p->threads[0]) {
        free(p->threads);
        free(p);
        schedRelease();
        return -ENOMEM;
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
    p->threads[0]->signalMask = t->signalMask;

    // NOTE: fork() only clones one thread, which is why we're not cloning the
    // entire process memory, but just the calling thread
    p->pages = t->pages;

    if(!p->threads[0]->context || !p->threads[0]->signalContext) {
        free(p->threads[0]);
        free(p->threads);
        free(p);
        schedRelease();
        return -ENOMEM;
    }

    // and clone the parent's context
    if(!platformCloneContext(p->threads[0]->context, t->context)) {
        free(p->threads[0]->context);
        free(p->threads[0]);
        free(p->threads);
        free(p);
        schedRelease();
        return -ENOMEM;
    }

    // clone signal handlers
    p->threads[0]->signals = signalClone(t->signals);

    // clone I/O descriptors
    Process *parent = getProcess(t->pid);
    if(parent) {
        memcpy(p->io, parent->io, sizeof(IODescriptor) * MAX_IO_DESCRIPTORS);
        p->iodCount = parent->iodCount;
        p->umask = parent->umask;

        // increment reference counts for file and socket descriptors and close
        // those flagged with O_CLOFORK
        for(int i = 0; i < MAX_IO_DESCRIPTORS; i++) {
            if(p->io[i].valid) {
                if(p->io[i].flags & O_CLOFORK) {
                    p->io[i].valid = 0;
                    p->io[i].data = NULL;
                    p->io[i].flags = 0;
                    continue;
                }

                switch(p->io[i].type) {
                case IO_FILE:
                    FileDescriptor *file = p->io[i].data;
                    file->refCount++;
                    break;
                case IO_SOCKET:
                    SocketDescriptor *socket = p->io[i].data;
                    socket->refCount++;
                    break;
                }
            }
        }

        // clone working directory
        strcpy(p->cwd, parent->cwd);

        // clone command line and process name
        strcpy(p->name, parent->name);
        strcpy(p->command, parent->command);

        // and process group
        p->pgrp = parent->pgrp;

        // if we made this far then the creation was successful
        // list the child process as a child of the parent
        Process **newChildren = realloc(parent->children, parent->childrenCount+1);
        if(!newChildren) {
            // we can't add the child to the parent's list
            platformCleanThread(p->threads[0]->context, p->threads[0]->highest);
            free(p);
            return -ENOMEM;
        }

        parent->children = newChildren;
        parent->children[parent->childrenCount] = p;
        parent->childrenCount++;
    }

    processes++;
    threads++;
    schedAdjustTimeslice();

    // and we're done - return zero to the child
    platformSetContextStatus(p->threads[0]->context, 0);
    schedRelease();
    return pid;     // and PID to the parent
}
