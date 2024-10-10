/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

#pragma once

#include <sys/types.h>

/* Signal Handler Macros */
#define SIG_DFL         (void (*)(int))0
#define SIG_ERR         (void (*)(int))1
#define SIG_HOLD        (void (*)(int))2
#define SIG_IGN         (void (*)(int))3

/* ISO C Signals */
#define SIGABRT         0
#define SIGFPE          1
#define SIGILL          2
#define SIGINT          3
#define SIGSEGV         4
#define SIGTERM         5

/* POSIX Extension Signals */
#define SIGALRM         6
#define SIGBUS          7
#define SIGCHLD         8
#define SIGCONT         9
#define SIGHUP          10
#define SIGKILL         11
#define SIGPIPE         12
#define SIGQUIT         13
#define SIGSEGV         14
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

typedef volatile uint32_t sig_atomic_t;
