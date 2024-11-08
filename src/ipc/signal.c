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

    Thread *dest = getThread(pid);
    if(!dest) return -ESRCH;
    if(!sig) return 0;  // verified that pid exists

    return platformSendSignal(t, dest, sig);
}