/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct SyscallRequest {
    bool busy;

    uint64_t function;
    uint64_t params[4];
    uint64_t ret;           // return value from the kernel to the program

    void *thread;
    struct SyscallRequest *next;
} SyscallRequest;

void syscallHandle();
