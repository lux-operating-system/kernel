/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <kernel/sched.h>

#define MAX_SYSCALL             49

typedef struct SyscallRequest {
    bool busy, queued, unblock;
    bool external;          // set for syscalls that are handled in user space
    bool retry;             // for async syscalls

    uint64_t requestID;     // unique random ID for user space syscalls
    uint64_t function;
    uint64_t params[4];
    uint64_t ret;           // return value from the kernel to the program

    struct Thread *thread;
    struct SyscallRequest *next;
} SyscallRequest;

void syscallHandle();
SyscallRequest *syscallEnqueue(SyscallRequest *);
SyscallRequest *syscallDequeue();
SyscallRequest *getSyscall(pid_t);
int syscallProcess();

/* dispatch table */
extern void (*syscallDispatchTable[])(SyscallRequest *);
