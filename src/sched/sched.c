/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <platform/platform.h>
#include <platform/lock.h>
#include <platform/context.h>
#include <kernel/sched.h>
#include <kernel/signal.h>
#include <kernel/logger.h>

static bool scheduling = false;
int processes, threads;
static lock_t lock = LOCK_INITIAL;
static uint8_t *pidBitmap;
static Process *first;       // first process in the linked list
static Process *last;
static pid_t lumen;          // we'll need this to adopt orphaned processes
static pid_t kernel;
static Thread *kthread;      // main kernel thread

/* schedInit(): initializes the scheduler */

void schedInit() {
    pidBitmap = calloc(1, (MAX_PID + 7) / 8);
    if(!pidBitmap) {
        KERROR("could not allocate memory for scheduler\n");
        while(1);
    }

    processes = 0;
    threads = 0;
    first = NULL;
    last = NULL;

    pidBitmap[0] = 1;   // PID zero is reserved and cannot be used
    KDEBUG("scheduler initialized\n");
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

/* kthreadCreate(): spawns a new kernel thread
 * params: entry - entry point of the thread
 * params: arg - argument to be passed to the thread
 * returns: thread number, zero on failure
 */

pid_t kthreadCreate(void *(*entry)(void *), void *arg) {
    acquireLockBlocking(&lock);
    pid_t tid = allocatePid();
    if(!tid) {
        KWARN("unable to allocate a PID, maximum processes running?\n");
        return 0;
    }

    Process *p;

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

        p = first;
    } else {
        p = first;
        while(p->next) {
            p = p->next;
        }

        p->next = calloc(1, sizeof(Process));
        p = p->next;
        if(!p) {
            KERROR("failed to allocate memory for kernel thread\n");
            releaseLock(&lock);
            return 0;
        }
    }

    p->pid = tid;
    p->parent = 0;
    p->user = 0;      // root
    p->group = 0;
    p->threadCount = 1;
    p->childrenCount = 0;
    p->children = NULL;
    strcpy(p->command, "kernel");

    p->threads = calloc(1, sizeof(Thread *));
    if(!p->threads) {
        KERROR("failed to allocate memory for kernel thread\n");
        free(p);
        if(!processes) first = NULL;
        releaseLock(&lock);
        return 0;
    }

    p->threads[0] = calloc(1, sizeof(Thread));
    if(!p->threads[0]) {
        KERROR("failed to allocate memory for kernel thread\n");
        free(p->threads);
        free(p);
        if(!processes) first = NULL;
        releaseLock(&lock);
        return 0;
    }

    p->threads[0]->status = THREAD_QUEUED;
    p->threads[0]->pid = tid;
    p->threads[0]->tid = tid;
    //p->threads[0]->time = PLATFORM_TIMER_FREQUENCY;
    p->threads[0]->next = NULL;
    p->threads[0]->context = calloc(1, PLATFORM_CONTEXT_SIZE);
    if(!p->threads[0]->context) {
        KERROR("failed to allocate memory for thread context\n");
        free(p->threads[0]);
        free(p->threads);
        free(p);
        if(!processes) first = NULL;
        releaseLock(&lock);
        return 0;
    }

    if(!platformCreateContext(p->threads[0]->context, PLATFORM_CONTEXT_KERNEL, (uintptr_t)entry, (uintptr_t)arg)) {
        KERROR("failed to create kernel thread context\n");
        free(p->threads[0]->context);
        free(p->threads[0]);
        free(p->threads);
        free(p);
        if(!processes) first = NULL;
        releaseLock(&lock);
        return 0;
    }

    KDEBUG("spawned kernel thread with PID %d\n", tid);
    last = p;

    processes++;
    threads++;

    schedAdjustTimeslice();
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

    return NULL;
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

/* schedTimer(): scheduler timer main function
 * params: none
 * returns: remaining time in milliseconds
 */

uint64_t schedTimer() {
    if(!scheduling || !processes || !threads || !first || !last) {
        return 1;
    }

    //if(!acquireLock(&lock)) return 1;

    // decrement the time slice of the current thread
    uint64_t time;
    Thread *t = getThread(getTid());
    if(!t) {
        time = 0;
    } else {
        if(t->time) t->time--;  // prevent underflows
        time = t->time;
    }

    // and that of sleeping threads too
    schedSleepTimer();

    //releaseLock(&lock);
    return time;
}

/* schedBusy(): determines if there are queued threads
 * params: none
 * returns: true/false
 */

bool schedBusy() {
    Process *qp = first;
    Thread *qt = NULL;
    while(qp) {
        if(qp->threads && qp->threadCount) qt = qp->threads[0];
        while(qt) {
            if(qt->status == THREAD_QUEUED) return true;
            qt = qt->next;
        }

        qp = qp->next;
    }

    return false;
}

/* schedule(): determines the next thread to run and performs a context switch
 * params: none
 * returns: nothing
 */

void schedule() {
    if(!scheduling || !processes || !threads) return;

    //acquireLockBlocking(&lock);
    if(!acquireLock(&lock)) return;
    setLocalSched(false);

    Process *p = platformGetProcess();
    Thread *t = platformGetThread();
    Thread *current = t;
    int cpu = platformWhichCPU();   // cpu index
    int rounds = 0;

    if(!p || !t) p = first;         // initial scheduling event

    while(rounds < 2) {
        if(!p->threadCount || !p->threads) {
            p = p->next;
            continue;
        }

        t = p->threads[0];

        while(t) {
            if(t->status == THREAD_QUEUED) {
                if(current && (current->status == THREAD_RUNNING))
                    current->status = THREAD_QUEUED;

                signalHandle(t);

                if(t->status == THREAD_QUEUED) {
                    // check status again because the signal handler may terminate a thread
                    t->status = THREAD_RUNNING;
                    t->time = schedTimeslice(t, t->priority);
                    t->cpu = cpu;
                    releaseLock(&lock);
                    platformSwitchContext(t);
                }
            }

            t = t->next;
        }

        p = p->next;
        if(!p) {
            // circular queue
            p = first;
            rounds++;
        }
    }

    releaseLock(&lock);
    return;
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

uint64_t schedTimeslice(Thread *t, int p) {
    if(!p) p = PRIORITY_NORMAL;
    uint64_t time = p * SCHED_TIME_SLICE;
    t->priority = p;
    return time;
}

/* schedAdjustTimeslice(): adjusts the timeslices of all queued threads */

void schedAdjustTimeslice() {
    Process *p = first;
    Thread *t = first->threads[0];

    while(p) {
        while(t) {
            if(t->status == THREAD_QUEUED || t->status == THREAD_BLOCKED) {
                t->time = schedTimeslice(t, t->priority);
            }

            t = t->next;
        }

        p = p->next;
        if(p && p->threadCount && p->threads) {
            t = p->threads[0];
        } else {
            t = NULL;
        }
    }
}

/* setScheduling(): enables or disables the scheduler
 * params: s - true/false
 * returns: nothing
 */

void setScheduling(bool s) {
    scheduling = s;
}

/* blockThread(): blocks a thread until a syscall request is handled
 * params: t - thread structure
 * returns: nothing
 */

void blockThread(Thread *t) {
    t->status = THREAD_BLOCKED;
    t->time = schedTimeslice(t, t->priority);
}

/* unblockThread(): unblocks a thread after a syscall was handled
 * params: t - thread structure
 * returns: nothing
 */

void unblockThread(Thread *t) {
    t->status = THREAD_QUEUED;      // the scheduler will eventually run it
}

/* yield(): gives up control of a thread and puts it back in the queue
 * params: t - thread in question
 * returns: zero
 */

int yield(Thread *t) {
    t->status = THREAD_QUEUED;
    t->time = schedTimeslice(t, t->priority);
    return 0;
}

/* getProcessQueue(): returns the process queue 
 * params: none
 * returns: linked list to the processes
 */

Process *getProcessQueue() {
    return first;
}

/* setLumenPID(): saves the PID of lumen 
 * params: pid - process ID
 * returns: nothing
 */

void setLumenPID(pid_t pid) {
    lumen = pid;
    KDEBUG("started lumen with pid %d\n", pid);
}

/* getLumenPID(): returns the PID of lumen
 * params: none
 * returns: process ID
 */

pid_t getLumenPID() {
    return lumen;
}

/* setKernelPID(): saves the main kernel thread's PID
 * params: pid - process ID
 * returns: nothing
 */

void setKernelPID(pid_t pid) {
    kernel = pid;
    kthread = getThread(pid);
}

/* getKernelPID(): returns the kernel's PID
 * params: none
 * returns: process ID
 */

pid_t getKernelPID() {
    return kernel;
}

/* getKernelThread(): returns the kernel's thread structure
 * params: none
 * returns: pointer to thread structure
 */

Thread *getKernelThread() {
    return kthread;
}