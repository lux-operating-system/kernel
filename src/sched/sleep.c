/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

#include <stdlib.h>
#include <string.h>
#include <kernel/sched.h>
#include <kernel/logger.h>

static Thread **sleepingThreads = NULL;   // list of sleeping threads
static int sleepingCount = 0;

/* msleep(): pauses thread execution for a duration of time in milliseconds
 * params: t - thread to be paused
 * params: msec - minimum number of milliseconds to pause
 * returns: zero
 */

unsigned long msleep(Thread *t, unsigned long msec) {
    if(!msec) return 0;

    schedLock();
    t->status = THREAD_SLEEP;
    t->time = msec;

    sleepingCount++;
    sleepingThreads = realloc(sleepingThreads, sizeof(Thread *) * sleepingCount);
    if(!sleepingThreads) {
        KERROR("failed to allocate memory to put thread %d to sleep\n", t->tid);
        t->status = THREAD_QUEUED;  // failed sleep (?)
        schedRelease();
        return 0;
    }

    sleepingThreads[sleepingCount-1] = t;
    schedRelease();
    return 0;
}

/* schedSleepTimer(): ticks sleeping threads and wakes them if the duration has elapsed
 * params: none
 * returns: nothing
 */

void schedSleepTimer() {
    // we don't need to lock the scheduler because it is already locked by
    // schedTimer() which calls this timer
    if(!sleepingCount || !sleepingThreads) return;

    for(int i = 0; i < sleepingCount; i++) {
        if(sleepingThreads[i] && sleepingThreads[i]->status == THREAD_SLEEP) {
            sleepingThreads[i]->time--;

            if(!sleepingThreads[i]->time) {
                // duration has elapsed, wake this thread
                sleepingThreads[i]->status = THREAD_QUEUED;
                sleepingThreads[i]->time = schedTimeslice(sleepingThreads[i], sleepingThreads[i]->priority);

                sleepingThreads[i] = NULL;

                if(i != (sleepingCount-1)) {
                    memmove(&sleepingThreads[i], &sleepingThreads[i+1], (sleepingCount-i) * sizeof(Thread *));
                }

                i--;
                sleepingCount--;
            }
        }
    }

    if(!sleepingCount) {
        free(sleepingThreads);
        sleepingThreads = NULL;
    }
}