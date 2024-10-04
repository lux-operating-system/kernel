/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

#include <platform/platform.h>
#include <platform/context.h>
#include <kernel/sched.h>
#include <kernel/logger.h>

/* terminateThread(): helper function to terminate a thread
 * params: t - thread to exit
 * params: status - exit code
 * params: normal - normal vs abnormal termination
 * returns: nothing
 */

void terminateThread(Thread *t, int status, bool normal) {
    status &= 0xFF;             // POSIX specifies signed 8-bit exit codes

    t->status = THREAD_ZOMBIE;  // zombie status until the exit status is read
    t->normalExit = normal;
    t->exitStatus = status;     // this should technically only be valid for normal exit

    if(normal) t->exitStatus |= EXIT_NORMAL;

    // lumen can never terminate
    if(t->pid == getLumenPID() || t->tid == getLumenPID()) {
        KPANIC("kernel panic: lumen (pid %d) terminated %snormally with exit status %d\n", getLumenPID(), normal ? "" : "ab", status);
        KPANIC("halting because there is nothing to do\n");
        while(1);
    }

    // check if this was the last thread in its process
    Process *p = getProcess(t->pid);
    if(!p) {
        KWARN("pid %d from tid %d returned null pointer\n", t->pid, t->tid);
        return;
    }

    // if all threads are zombies, mark the process as a whole as a zombie
    // and then mark its children as orphans
    for(int i = 0; i < p->threadCount; i++) {
        if(p->threads[i]->status == THREAD_ZOMBIE) {
            p->zombie = true;
        } else {
            p->zombie = false;
            break;
        }
    }

    if(p->zombie && p->childrenCount && p->children) {
        // parent process is now a zombie, mark all children as orphans before
        // the parent status is read and it quits
        for(int i = 0; i < p->childrenCount; i++) {
            if(p->children[i]) {
                p->children[i]->orphan = true;
                p->children[i]->parent = getLumenPID(); // orphan processes are adopted by lumen
            }
        }
    }

    if(!normal) {
        KWARN("killed tid %d abnormally\n", t->tid);
    }
}

/* exit(): normally terminates the current running thread
 * params: t - thread to exit
 * params: status - exit code
 * returns: nothing
 */

void exit(Thread *t, int status) {
    // this is really a wrapper around a helper function to allow for normal
    // and abnormal termination in one place
    schedLock();
    terminateThread(t, status, true);
    schedRelease();
}

/* schedException(): exception handler for when a process causes a fault
 * params: pid - faulting process ID
 * params: tid - faulting thread ID
 * returns: 0 if the thread can be returned to, 1 if it was terminated
 */

int schedException(pid_t pid, pid_t tid) {
    // TODO: implement custom process exception handlers after signals and
    // allow them a chance at their own recovery
    Thread *t = getThread(tid);
    if(!t) {
        KERROR("faulting pid %d tid %d returned null pointer when trying to terminate\n");
        return 1;
    }

    terminateThread(t, -1, false);
    return 1;
}
