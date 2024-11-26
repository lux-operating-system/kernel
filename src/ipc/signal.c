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
#include <kernel/logger.h>
#include <platform/lock.h>
#include <platform/mmap.h>
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
    if(signum <= 0 || signum > MAX_SIGNAL)
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
    if(signum <= 0 || signum > MAX_SIGNAL)
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
    if(signum <= 0 || signum > MAX_SIGNAL)
        return -EINVAL;

    if(*set & (1 << signum)) return 1;
    else return 0;
}

/* signalDefaults(): sets up the default signal handlers for a thread
 * params: none
 * returns: pointer to the signal handler array, NULL on fail
 */

void *signalDefaults() {
    struct sigaction *ptr = malloc((MAX_SIGNAL+1) * sizeof(struct sigaction));
    if(!ptr) return NULL;

    for(int i = 0; i < MAX_SIGNAL; i++)
        ptr[i].sa_handler = SIG_DFL; // default
    
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

    void *new = malloc((MAX_SIGNAL+1) * sizeof(struct sigaction));
    if(!new) return NULL;

    return memcpy(new, h, (MAX_SIGNAL+1) * sizeof(struct sigaction));
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
        s->sender = t;
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

/* signalHandle(): checks the signal queue and invokes a signal handler
 * params: t - thread to check
 * returns: nothing
 */

void signalHandle(Thread *t) {
    if(t->handlingSignal || !t->signalQueue) return;

    acquireLockBlocking(&t->lock);

    // linked list structure
    SignalQueue *s = t->signalQueue;
    t->signalQueue = s->next;

    releaseLock(&t->lock);

    int signum = s->signum - 1;     // change to zero-based
    struct sigaction *handlers = (struct sigaction *) t->signals;
    uintptr_t handler = (uintptr_t) handlers[signum].sa_handler;
    int def = 0;

    free(s);
    
    switch(handler) {
    case (uintptr_t) SIG_IGN:
    case (uintptr_t) SIG_HOLD:
        return;
    case (uintptr_t) SIG_DFL:
        def = signalDefaultHandler(signum + 1);
        break;
    }

    switch(def) {
    case SIG_I:
    case SIG_C:
        return;
    case SIG_T:
    case SIG_A:
    case SIG_S:
        terminateThread(t, -1, true);
        break;
    default:
        t->handlingSignal = true;
        platformSendSignal(s->sender, t, s->signum);
        for(;;);
    }
}

/* sigaction(): manipulate a signal handler
 * params: t - calling thread
 * params: sig - signal number
 * params: act - new signal handler, NULL to query the current signal handler
 * params: oact - old signal handler, NULL if not requested
 * returns: zero on success, negative errno on fail
 */

int sigaction(Thread *t, int sig, const struct sigaction *act, struct sigaction *oact) {
    if(sig <= 0 || sig > MAX_SIGNAL) return -EINVAL;

    struct sigaction *handlers = (struct sigaction *) t->signals;

    if(!act) {
        // query signal handler
        if(!oact) return 0;
        memcpy(oact, &handlers[sig-1], sizeof(struct sigaction));
        return 0;
    }

    uintptr_t handler = (uintptr_t) act->sa_handler;
    if(handler != (uintptr_t) SIG_DFL && handler != (uintptr_t) SIG_IGN &&
        handler < USER_BASE_ADDRESS)
        return -EINVAL;

    acquireLockBlocking(&t->lock);

    // save the old signal handler if necessary
    if(oact)
        memcpy(oact, &handlers[sig-1], sizeof(struct sigaction));

    memcpy(&handlers[sig-1], act, sizeof(struct sigaction));
    releaseLock(&t->lock);
    return 0;
}