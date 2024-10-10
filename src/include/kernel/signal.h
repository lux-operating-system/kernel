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
#define SIGSEGV         15
#define SIGSTOP         16
#define SIGTSTP         17
#define SIGTTIN         18
#define SIGTTOU         19
#define SIGUSR1         20
#define SIGUSR2         21
#define SIGPOLL         22
#define SIGSYS          23
#define SIGTRAP         24
#define SIGURG          25
#define SIGVTALRM       26
#define SIGXCPU         27
#define SIGXFSZ         28

typedef volatile uint32_t sig_atomic_t;
