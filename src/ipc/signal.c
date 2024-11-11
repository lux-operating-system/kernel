/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <kernel/signal.h>
#include <kernel/sched.h>
#include <platform/lock.h>
#include <platform/platform.h>

/* Implementation of ISO C and POSIX Signals */

/* sigemptyset(): clears a set of signals
 * params: set - set of signals
 * returns: zero
 */

int sigemptyset(sigset_t *set) {
    *set = 0;
    return 0;
}

/* sigfillset(): fills a set of signals with all supported signals
 * params: set - set of signals
 * returns: zero
 */

int sigfillset(sigset_t *set) {
    *set = 0;

    for(int i = 0; i < MAX_SIGNAL; i++)
        *set |= (1 << i);

    return 0;
}

/* sigaddset(): adds a signal to a set of signals
 * params: set - set of signals
 * params: signum - signal to add
 * returns: zero on success
 */

int sigaddset(sigset_t *set, int signum) {
    if(signum < 0 || signum > MAX_SIGNAL)
        return -EINVAL;

    *set |= (1 << signum);
    return 0;
}

/* sigdelset(): removes a signal from a set of signals
 * params: set - set of signals
 * params: signum - signal to be removed
 * returns: zero on success
 */

int sigdelset(sigset_t *set, int signum) {
    if(signum < 0 || signum > MAX_SIGNAL)
        return -EINVAL;

    *set &= ~(1 << signum);
    return 0;
}

/* sigismember(): tests if a signal is in a set of signals
 * params: set - set of signals
 * params: signum - signal to be checked
 * returns: zero if absent, one if present, negative error code on fail
 */

int sigismember(sigset_t *set, int signum) {
    if(signum < 0 || signum > MAX_SIGNAL)
        return -EINVAL;

    if(*set & (1 << signum)) return 1;
    else return 0;
}

/* signalDefaults(): sets up the default signal handlers for a thread
 * params: none
 * returns: pointer to the signal handler array, NULL on fail
 */

void *signalDefaults() {
    uintptr_t *ptr = malloc((MAX_SIGNAL+1) * sizeof(uintptr_t));
    if(!ptr) return NULL;

    for(int i = 0; i < MAX_SIGNAL; i++)
        *ptr = (uintptr_t) SIG_DFL; // default
    
    return (void *) ptr;
}

/* signalDefaultHandler(): returns the default handler for a signal
 * params: signum - signal number
 * returns: default handler macro, zero on fail
 */

int signalDefaultHandler(int signum) {
    switch(signum) {
    case SIGABRT:
    case SIGBUS:
    case SIGFPE:
    case SIGILL:
    case SIGQUIT:
    case SIGSEGV:
    case SIGSYS:
    case SIGTRAP:
    case SIGXCPU:
    case SIGXFSZ:
        return SIG_A;

    case SIGALRM:
    case SIGHUP:
    case SIGINT:
    case SIGKILL:
    case SIGPIPE:
    case SIGTERM:
    case SIGUSR1:
    case SIGUSR2:
    case SIGPOLL:
    case SIGVTALRM:
        return SIG_T;

    case SIGCHLD:
    case SIGURG:
        return SIG_I;
    
    case SIGCONT:
        return SIG_C;
    
    default:
        return 0;
    }
}

/* signalClone(): clones the signal handlers of a thread
 * params: h - template signal handlers, NULL to use defaults
 * returns: pointer to the signal handler array, NULL on fail
 */

void *signalClone(const void *h) {
    if(!h) return signalDefaults();

    void *new = malloc((MAX_SIGNAL+1) * sizeof(uintptr_t));
    if(!new) return NULL;

    return memcpy(new, h, (MAX_SIGNAL+1) * sizeof(uintptr_t));
}

/* kill(): sends a signal to a process or thread
 * params: t - calling thread
 * params: pid - pid of the process/thread to send a signal to
 * pid > 0 refers to the process/thread whose PID/TID is exactly pid
 * pid == 0 refers to processes whose process group ID is that of the sender
 * pid == -1 refers to all processes that may receive the signal
 * pid < -1 refers to the process group whose ID is the abs(pid)
 * params: sig - signal to be sent, zero to test PID validitiy
 * returns: zero on success, negative error code on fail
 */

int kill(Thread *t, pid_t pid, int sig) {
    if(sig < 0 || sig > MAX_SIGNAL) return -EINVAL;

    pid_t group;
    Process *parent;
    Thread *dest;

    if(pid == -1) return -EPERM;    /* placeholder */

    if(pid < -1) group = -1 * pid;
    else if(!pid) group = t->pid;
    else group = 0;

    if(group) {
        parent = getProcess(group);
        if(!parent) return -ESRCH;
    } else {
        dest = getThread(pid);
        if(!dest) return -ESRCH;
    }

    if(!sig) return 0;  // null signal verifies that pid is valid

    if(group) {
        // send the signal to all threads of the parent as well as all threads
        // of all the children
        if(!parent->threads || !parent->threadCount) return 0;

        for(int i = 0; i < parent->threadCount; i++) {
            dest = parent->threads[i];
            if(dest) {
                int status = kill(t, dest->tid, sig);
                if(status) return status;
            }
        }

        if(!parent->children || !parent->childrenCount) return 0;

        Process *child;
        for(int i = 0; i < parent->childrenCount; i++) {
            child = parent->children[i];
            if(!child || !child->threads || !child->threadCount)
                continue;

            for(int j = 0; j < child->threadCount; j++) {
                dest = child->threads[j];
                if(dest) {
                    int status = kill(t, dest->tid, sig);
                    if(status) return status;
                }
            }
        }
    } else {
        // send the signal to the exact thread specified by pid
        SignalQueue *s = calloc(1, sizeof(SignalQueue));
        if(!s) return -ENOMEM;
        
        s->signum = sig;
        s->next = NULL;

        acquireLockBlocking(&dest->lock);

        SignalQueue *q = dest->signalQueue;
        if(!q) {
            dest->signalQueue = s;
        } else {
            while(q->next) q = q->next;
            q->next = s;
        }

        releaseLock(&dest->lock);
    }

    return 0;
}