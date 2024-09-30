/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Platform-Specific Code for x86_64
 */

#pragma once

#include <stdint.h>
#include <kernel/sched.h>

/* Thread Context for x86_64 */

typedef struct {
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rbp;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rbx;
    uint64_t rax;
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
} __attribute__((packed)) ThreadGPR;

typedef struct {
    uint8_t sse[512];       // fxsave

    // paging base will only be switched between processes
    // threads under the same process will share the same address space
    uint64_t cr3;

    ThreadGPR regs;         // register state

    int iopl;               // set to 1 if I/O port privileges have been modified
    uint8_t ioports[8192];  // I/O port privileges
} __attribute__((packed)) ThreadContext;

void *platformCreateContext(void *, int, uintptr_t, uintptr_t);
int platformSetContext(Thread *, uintptr_t, uintptr_t, const char **, const char **);

#define PLATFORM_CONTEXT_SIZE       sizeof(ThreadContext)

#define PLATFORM_CONTEXT_KERNEL     0
#define PLATFORM_CONTEXT_USER       1
#define PLATFORM_THREAD_STACK       65536
