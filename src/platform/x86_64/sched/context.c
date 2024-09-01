/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Platform-Specific Code for x86_64
 */

/* This file contains the context manipulation for the scheduler */
/* The definition of context varies by CPU architecture, so hide the difference
 * behind this abstraction layer. */

#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <platform/context.h>
#include <platform/platform.h>
#include <platform/smp.h>
#include <platform/x86_64.h>
#include <kernel/logger.h>
#include <kernel/sched.h>
#include <kernel/memory.h>

/* platformGetPid(): returns the PID of the process running on the current CPU
 * params: none
 * returns: process ID, zero if idle
 */

pid_t platformGetPid() {
    KernelCPUInfo *kinfo = getKernelCPUInfo();
    if(kinfo->process) return kinfo->process->pid;
    return 0;
}

/* platformGetTid(): returns the TID of the thread running on the current CPU
 * params: none
 * returns: thread ID, zero if idle
 */

pid_t platformGetTid() {
    KernelCPUInfo *kinfo = getKernelCPUInfo();
    if(kinfo->thread) return kinfo->thread->tid;
    return 0;
}

/* platformCreateContext(): creates the context for a new thread
 * params: ptr - pointer to the context structure
 * params: level - kernel/user space
 * params: entry - entry point of the thread
 * params: arg - argument to be passed to the thread
 * returns: pointer to the context structure, NULL on failure
 */

void *platformCreateContext(void *ptr, int level, uintptr_t entry, uintptr_t arg) {
    memset(ptr, 0, PLATFORM_CONTEXT_SIZE);

    ThreadContext *context = (ThreadContext *)ptr;
    context->regs.rip = entry;
    context->regs.rdi = arg;
    context->regs.rflags = 0x202;
    context->cr3 = (uint64_t)platformCloneKernelSpace();
    if(!context->cr3) return NULL;
    void *stack;

    if(level == PLATFORM_CONTEXT_KERNEL) {
        context->regs.cs = GDT_KERNEL_CODE << 3;
        context->regs.ss = GDT_KERNEL_DATA << 3;
        stack = calloc(1, PLATFORM_THREAD_STACK);
        if(!stack) return NULL;
        context->regs.rsp = (uint64_t)stack + PLATFORM_THREAD_STACK;

        // TODO: handle implicit thread termination by returning
        // to do so we would need to push a return function to the thread's stack

        return ptr;
    } else {
        context->regs.cs = (GDT_USER_CODE << 3) | PRIVILEGE_USER;
        context->regs.ss = (GDT_USER_DATA << 3) | PRIVILEGE_USER;

        // stack will not be setup here for user processes because it depends
        // on the process's size and location and etc
        // same for entry point and args

        return ptr;
    }
}

/* platformSwitchContext(): switches the current thread context 
 * params: t - thread to switch to
 * returns: doesn't return
 */

void platformSwitchContext(Thread *t) {
    KernelCPUInfo *kinfo = getKernelCPUInfo();
    //KDEBUG("switching to thread %d on CPU %d\n", t->tid, getKernelCPUInfo()->cpuIndex);

    kinfo->thread = t;
    kinfo->process = getProcess(t->pid);
    platformLoadContext(t->context);
}

/* platformUseContext(): switches to the paging context of a thread
 * params: ptr - thread context
 * returns: zero
 */

int platformUseContext(void *ptr) {
    ThreadContext *ctx = (ThreadContext *)ptr;
    writeCR3(ctx->cr3);
    return 0;
}

/* platformSetContext(): sets up the context for a user space thread
 * params: t - thread
 * params: entry - entry point
 * params: highest - highest address loaded
 * params: argv - arguments to be passed
 * params: envp - environmental variables to be passed
 * returns: zero on success
 */

int platformSetContext(Thread *t, uintptr_t entry, uintptr_t highest, const char **argv, const char **envp) {
    /* this sets up an entry point for the thread that's something like
     * void _start(const char **argv, const char **envp) */

    ThreadContext *ctx = (ThreadContext *)t->context;
    ctx->regs.rip = entry;
    ctx->regs.rdi = (uint64_t)argv;
    ctx->regs.rsi = (uint64_t)envp;

    // allocate a user stack
    uintptr_t base = highest;
    while(base % PAGE_SIZE) {
        base++;
    }
    uintptr_t limit = base + PAGE_SIZE + PLATFORM_THREAD_STACK;
    size_t pages = (PLATFORM_THREAD_STACK+PAGE_SIZE-1)/PAGE_SIZE;
    pages++;

    uintptr_t stack = vmmAllocate(base, limit, pages, VMM_WRITE | VMM_USER);
    if(!stack) return -1;
    memset((void *)stack, 0, PLATFORM_THREAD_STACK + PAGE_SIZE);

    stack += PLATFORM_THREAD_STACK;
    ctx->regs.rsp = stack;

    t->highest = stack;     // requisite to sbrk() someday
    return 0;
}