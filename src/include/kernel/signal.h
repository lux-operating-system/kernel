/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

#pragma once

#include <sys/types.h>
#include <kernel/sched.h>

/* Signal Handler Macros */
#define SIG_DFL         (void (*)(int))0
#define SIG_ERR         (void (*)(int))1
#define SIG_HOLD        (void (*)(int))2
#define SIG_IGN         (void (*)(int))3

/* ISO C Signals */
#define SIGABRT         1
#define SIGFPE          2
#define SIGILL          3
#define SIGINT          4
#define SIGSEGV         5
#define SIGTERM         6

/* POSIX Extension Signals */
#define SIGALRM         7
#define SIGBUS          8
#define SIGCHLD         9
#define SIGCONT         10
#define SIGHUP          11
#define SIGKILL         12
#define SIGPIPE         13
#define SIGQUIT         14
#define SIGSTOP         15
#define SIGTSTP         16
#define SIGTTIN         17
#define SIGTTOU         18
#define SIGUSR1         19
#define SIGUSR2         20
#define SIGPOLL         21
#define SIGSYS          22
#define SIGTRAP         23
#define SIGURG          24
#define SIGVTALRM       25
#define SIGXCPU         26
#define SIGXFSZ         27

#define MAX_SIGNAL      27

typedef volatile uint32_t sig_atomic_t;
typedef uint64_t sigset_t;

typedef struct {
    int si_signo, si_code, si_errno, si_status;
    pid_t si_pid;
    uid_t si_uid;
    void *si_addr;
    long si_band;
} siginfo_t;

struct sigaction {
    union {
        void (*sa_handler)(int);
        void (*sa_sigaction)(int, siginfo_t *, void *);
    } handler;

    sigset_t sa_mask;
    int sa_flags;
};

int sigemptyset(sigset_t *);
int sigfillset(sigset_t *);
int sigaddset(sigset_t *, int);
int sigdelset(sigset_t *, int);
int sigismember(sigset_t *, int);

void *signalDefaults();
void *signalClone(const void *);

int kill(Thread *, pid_t, int);
int sigaction(Thread *, int, const struct sigaction *, struct sigaction *);
