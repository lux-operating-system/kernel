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
#include <kernel/logger.h>

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
    context->cr3 = (uint64_t)platformCloneKernelSpace();
    if(!context->cr3) return NULL;
    void *stack;

    if(level == PLATFORM_CONTEXT_KERNEL) {
        stack = malloc(PLATFORM_THREAD_STACK);
        if(!stack) return NULL;
        context->regs.rsp = (uint64_t)stack + PLATFORM_THREAD_STACK;

        // TODO: handle implicit thread termination by returning
        // to do so we would need to push a return function to the thread's stack

        return ptr;
    } else {
        KDEBUG("TODO: implement user-space threads\n");
        while(1);
    }
}
