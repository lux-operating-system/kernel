/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

#include <string.h>
#include <stdlib.h>
#include <kernel/sched.h>
#include <platform/platform.h>

/* threadCleanup(): frees all memory associated with a thread and removes it
 * from the run queues
 * params: t - thread structure
 * returns: nothing
 */

void threadCleanup(Thread *t) {
    platformCleanThread(t->context, t->highest);

    /* TODO: properly re-implement this after implementing per-CPU run queue */
}