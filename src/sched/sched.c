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
int processes, threads;
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

void schedLock() {
    acquireLockBlocking(&lock);
}

void schedRelease() {
    releaseLock(&lock);
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

        if(!platformCreateContext(first->threads[0]->context, PLATFORM_CONTEXT_KERNEL, (uintptr_t)entry, (uintptr_t)arg)) {
            KERROR("failed to create kernel thread context\n");
            free(first->threads[0]->context);
            free(first->threads[0]);
            free(first->threads);
            free(first);
            first = NULL;
            releaseLock(&lock);
            return 0;
        }

        KDEBUG("spawned kernel thread with PID %d\n", tid);
        last = first;
        processes++;
        threads++;
        releaseLock(&lock);
        return tid;
    } else {
        KDEBUG("TODO: implement multithreading in non-kernel threads\n");
        while(1);
    }

    releaseLock(&lock);
    return tid;
}

/* getProcess(): returns the process structure associated with a PID
 * params: pid - process ID
 * returns: pointer to the process structure, NULL on failure
 */

Process *getProcess(pid_t pid) {
    if(!pid || !first) return NULL;

    Process *p = first;
    do {
        if(p->pid == pid) return p;
        p = p->next;
    } while(p);

    return NULL;
}

/* getThread(): returns the thread structure associated with a TID
 * params: tid - thread ID
 * returns: pointer to the thread structure, NULL on failure
 */

Thread *getThread(pid_t tid) {
    if(!tid || !first) return NULL;

    Process *p = first;
    Thread *t;
    do {
        if(p->threadCount && p->threads) {
            for(int i = 0; i < p->threadCount; i++) {
                t = p->threads[i];
                if(t && t->tid == tid) return t;
            }
        }

        p = p->next;
    } while(p);
}

/* getPid(): returns the current running process
 * params: none
 * returns: process ID
 */

pid_t getPid() {
    return platformGetPid();
}

/* getTid(): returns the current running thread
 * params: none
 * returns: thread ID
 */

pid_t getTid() {
    return platformGetTid();
}

/* schedTimer(): decrements and returns the time slice of the running thread
 * params: none
 * returns: remaining time in milliseconds
 */

uint64_t schedTimer() {
    if(!scheduling || !processes || !threads || !first || !last) {
        return ~0;
    }

    if(!acquireLock(&lock)) return ~0;

    uint64_t time;
    Thread *t = getThread(getTid());
    if(!t) {
        time = 0;
    } else {
        t->time--;
        time = t->time;
    }

    releaseLock(&lock);
    return time;
}

/* schedule(): determines the next thread to run and performs a context switch
 * params: none
 * returns: nothing
 */

void schedule() {
    acquireLockBlocking(&lock);

    /* determine the next process to be run */
    int cpu = platformWhichCPU();
    pid_t pid = getPid();
    pid_t tid = getTid();
    Process *p = getProcess(pid);
    Thread *t = getThread(tid);

    if(!pid || !tid || !p || !t) {
        // special case for first process
        p = first;
        t = p->threads[0];

        if(t->status == THREAD_QUEUED) {
            KDEBUG("initial for %d on CPU %d\n", t->tid, cpu);
            t->status = THREAD_RUNNING;
            t->cpu = cpu;
            releaseLock(&lock);
            platformSwitchContext(t);
        }
    }

    // nope, now we try to look for a process to run
    KDEBUG("searching for queued threads on cpu %d\n", cpu);
    Thread *current = getThread(getTid());

    while(p) {
        while(t) {
            if(t->status == THREAD_QUEUED) {
                if(current) {
                    current->status = THREAD_QUEUED;
                }

                KDEBUG("switch to %d on %d\n", t->tid, cpu);
                t->status = THREAD_RUNNING;
                t->cpu = cpu;
                releaseLock(&lock);
                platformSwitchContext(t);
            }

            //KDEBUG("checking TID %d\n", t->tid);
            t = t->next;
        }

        p = p->next;
        if(p && p->threadCount && p->threads) {
            //KDEBUG("checking PID %d\n", p->pid);
            t = p->threads[0];
        }
    }

    releaseLock(&lock);
    KDEBUG("didn't find any queued threads on %d\n", cpu);
}

/* processCreate(): creates a blank process
 * params: none
 * returns: process ID, zero on failure
 */

pid_t processCreate() {
    /* IMPORTANT: here we do NOT lock because this is never directly called */
    /* this will be called from fork(), exec(), etc, which will take care of
     * the locking and preserving sanity */

    pid_t pid = allocatePid();
    if(!pid) {
        return 0;
    }

    Process *process = first;
    while(process->next) {
        process = process->next;
    }

    process->next = calloc(1, sizeof(Process));
    if(!process->next) {
        KERROR("failed to allocate memory for new process\n");
        return 0;
    }

    process = process->next;
    Process *current = getProcess(getPid());
    if(current) {
        current->children = realloc(current->children, sizeof(Process *) * current->childrenCount+1);
        if(!current->children) {
            KERROR("failed to allocate memory for new process\n");
            free(process);
            return 0;
        }

        // add to the list of children processes
        current->children[current->childrenCount] = process;
        current->childrenCount++;
    }

    // identify the process
    process->pid = pid;
    process->parent = getPid();
    process->user = 0;          // TODO
    process->group = 0;         // TODO

    // env and command line will be taken care of by fork() or exec()

    process->threadCount = 0;
    process->childrenCount = 0;

    return pid;
}

/* threadUseContext(): switches to the paging context of a thread
 * params: tid - thread ID
 * returns: zero on success
 */

int threadUseContext(pid_t tid) {
    Thread *t = getThread(tid);
    if(!t) return -1;

    return platformUseContext(t->context);
}

/* schedTimeslice(): allocates a time slice for a thread
 * params: t - thread structure
 * params: p - priority level (0 = highest, 3 = lowest)
 * returns: time slice in milliseconds accounting for CPU count
 */

uint64_t schedTimeslice() {
    
}