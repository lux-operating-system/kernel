/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Platform-Specific Code for x86_64
 */

#include <kernel/sched.h>
#include <platform/platform.h>
#include <platform/context.h>
#include <platform/smp.h>

/* this is called when the kernel threads are idle
 * it essentially tries to queue another thread to do something more useful
 * with the CPU time, else it returns (and then idle.asm halts the CPU to save
 * power since there are no queued tasks) */

void kernelYield(void *stack) {
    // save the kernel thread's context
    KernelCPUInfo *info = getKernelCPUInfo();
    if(info->thread && info->thread->context) {
        platformSaveContext(info->thread->context, stack);
    }

    schedule();
}