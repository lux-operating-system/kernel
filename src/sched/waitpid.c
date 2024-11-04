/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

#include <errno.h>
#include <kernel/sched.h>

/* processStatus(): helper function to return the status of a process
 * params: p - process to check
 * params: status - buffer to store the status
 * returns: PID on success, zero if no status is available
 */

static pid_t processStatus(Process *p, int *status) {
    if(!p) return -ESRCH;
    if(!p->threadCount || !p->threads) return 0;

    // iterate over the threads of the process and return the first one with a
    // valid exit status code
    for(int i = 0; i < p->threadCount; i++) {
        Thread *t = p->threads[i];
        if(!t) continue;

        if((!t->clean) && (t->status == THREAD_ZOMBIE)) {
            t->clean = true;
            *status = t->exitStatus;
            pid_t pid = t->tid;

            // free the thread structure
            threadCleanup(t);
            return pid;
        }
    }

    return 0;   // no data available
}

/* waitpid(): polls the status of a process (group)
 * params: t - calling thread
 * params: pid - pid of the process (group) to poll
 * pid == -1 refers to all children processes of the caller
 * pid == 0 refers to any process in the same process group of the caller
 * pid < -1 refers to any process in the process group referred to by abs(pid)
 * pid > 0 refers to the process whose PID is exactly pid
 * params: status - buffer to store the status at
 * params: options - modifying flags
 * returns: zero if no status is available
 * returns: PID of the process if status is available
 * returns: negative error code on error
 */

pid_t waitpid(Thread *t, pid_t pid, int *status, int options) {
    Process *p = getProcess(t->pid);
    if(!p) return -ESRCH;

    schedLock();

    if(pid > 0) {
        // case for one specific process
        pid = processStatus(getProcess(pid), status);
        schedRelease();
        return pid;
    }

    if(pid < -1) {
        pid *= -1;
        p = getProcess(pid);
        if(!p) {
            schedRelease();
            return -ESRCH;
        }
    }

    // iterate over children processes and threads
    if(!p->childrenCount || !p->children) {
        schedRelease();
        return -ECHILD;
    }

    for(int i = 0; i < p->childrenCount; i++) {
        Process *child = p->children[i];
        if(!child) continue;
    
        pid = processStatus(child, status);
        if(pid) {
            // return pid here which will be valid for both error and success
            schedRelease();
            return pid;
        }
    }

    // no data is available
    schedRelease();
    return 0;
}