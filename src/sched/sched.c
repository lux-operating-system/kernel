/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <platform/platform.h>
#include <platform/lock.h>
#include <platform/context.h>
#include <kernel/sched.h>
#include <kernel/logger.h>

static bool scheduling = false;
static int processes, threads;
static lock_t lock = LOCK_INITIAL;
static uint8_t *pidBitmap;
static Process *first;       // first process in the linked list
static Process *last;

/* schedInit(): initializes the scheduler */

void schedInit() {
    processes = 0;
    threads = 0;
    first = NULL;
    last = NULL;

    pidBitmap = calloc(1, (MAX_PID + 7) / 8);
    if(!pidBitmap) {
        KERROR("could not allocate memory for scheduler\n");
        while(1);
    }

    pidBitmap[0] = 1;   // PID zero is reserved and cannot be used
    scheduling = true;
    KDEBUG("scheduler enabled\n");
}

/* pidIsUsed(): returns the use status of a PID
 * params: pid - process ID
 * returns: true/false
 */

bool pidIsUsed(pid_t pid) {
    if(pid >= MAX_PID) return true;

    size_t byte = pid / 8;
    size_t bit = pid % 8;
    
    return (pidBitmap[byte] >> bit) & 1;
}

/* allocatePid(): allocates a new random PID 
 * params: none
 * returns: new PID, zero on failure
 */

pid_t allocatePid() {
    if(processes >= MAX_PID || threads >= MAX_PID) return 0;

    pid_t pid;
    do {
        pid = rand() % MAX_PID;
    } while(pidIsUsed(pid));

    size_t byte = pid / 8;
    size_t bit = pid % 8;

    pidBitmap[byte] |= (1 << bit);
    return pid;
}

/* releasePid(): releases a PID 
 * params: pid - process ID
 * returns: nothing
 */

void releasePid(pid_t pid) {
    if(pid >= MAX_PID) return;

    size_t byte = pid / 8;
    size_t bit = pid % 8;

    pidBitmap[byte] &= ~(1 << bit);
}

/* threadCreate(): spawns a new thread
 * params: entry - entry point of the thread
 * params: arg - argument to be passed to the thread
 * returns: thread number, zero on failure
 */

pid_t threadCreate(void *(*entry)(void *), void *arg) {
    acquireLockBlocking(&lock);
    pid_t tid = allocatePid();
    if(!tid) {
        KWARN("unable to allocate a PID, maximum processes running?\n");
        return 0;
    }

    /* special case for the kernel idle thread
     * this is the ONLY thread that runs in kernel space */
    if(!processes) {
        // creating initial kernel thread
        first = calloc(1, sizeof(Process));
        if(!first) {
            KERROR("failed to allocate memory for kernel thread\n");
            releaseLock(&lock);
            return 0;
        }

        first->pid = tid;
        first->parent = 0;
        first->user = 0;      // root
        first->group = 0;
        first->threadCount = 1;
        first->childrenCount = 0;
        first->children = NULL;

        first->threads = calloc(1, sizeof(Thread *));
        if(!first->threads) {
            KERROR("failed to allocate memory for kernel thread\n");
            free(first);
            first = NULL;
            releaseLock(&lock);
            return 0;
        }

        first->threads[0] = calloc(1, sizeof(Thread));
        if(!first->threads[0]) {
            KERROR("failed to allocate memory for kernel thread\n");
            free(first->threads);
            free(first);
            first = NULL;
            releaseLock(&lock);
            return 0;
        }

        first->threads[0]->status = THREAD_QUEUED;
        first->threads[0]->pid = tid;
        first->threads[0]->tid = tid;
        first->threads[0]->time = PLATFORM_TIMER_FREQUENCY;
        first->threads[0]->next = NULL;
        first->threads[0]->context = calloc(1, PLATFORM_CONTEXT_SIZE);
        if(!first->threads[0]->context) {
            KERROR("failed to allocate memory for thread context\n");
            free(first->threads[0]);
            free(first->threads);
            free(first);
            first = NULL;
            releaseLock(&lock);
            return 0;
        }

        KDEBUG("spawned kernel thread with PID %d\n", tid);
        platformCreateContext(first->threads[0]->context, PLATFORM_CONTEXT_KERNEL, (uintptr_t)entry, (uintptr_t)arg);
    }

    releaseLock(&lock);
    return tid;
}