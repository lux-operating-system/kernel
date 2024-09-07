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

#define MAX_SYSCALL                     12       // for now

typedef struct SyscallRequest {
    bool busy, queued, unblock;

    uint64_t function;
    uint64_t params[4];
    uint64_t ret;           // return value from the kernel to the program

    struct Thread *thread;
    struct SyscallRequest *next;
} SyscallRequest;

void syscallHandle();
SyscallRequest *syscallEnqueue(SyscallRequest *);
SyscallRequest *syscallDequeue();
int syscallProcess();

/* dispatch table */
extern void (*syscallDispatchTable[])(SyscallRequest *);
