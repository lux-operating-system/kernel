/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

/* Program Break Memory Manager */

#include <stdint.h>
#include <sys/types.h>
#include <platform/platform.h>
#include <kernel/sched.h>
#include <kernel/memory.h>

/* sbrk(): changes the size of the program's data segment by moving the program break
 * params: t - calling thread
 * params: delta - number of bytes to increment or decrement
 * returns: pointer to previous program break on success, -1 on fail
 */

void *sbrk(Thread *t, intptr_t delta) {
    intptr_t brk = t->highest;
    intptr_t newbrk = t->highest + delta;

    ssize_t diff = newbrk - brk;
    if(diff < 0) diff *= -1;

    size_t pages = (diff + PAGE_SIZE - 1) / PAGE_SIZE;

    if(!delta) return (void *)brk;

    Process *p = getProcess(t->pid);
    if(!p) return (void *)-1;

    if(delta > 0) {
        // thread is trying to allocate memory
        // we will be optimistic here and allocate virtual memory only, making
        // the physical mm work only upon access
        uintptr_t ptr = vmmAllocate(brk, USER_LIMIT_ADDRESS, pages, VMM_USER | VMM_WRITE);
        if(!ptr) {
            return (void *)-1;
        } else if(ptr != brk) {
            vmmFree(ptr, pages);
            return (void *)-1;
        }

        t->pages += pages;
        p->pages += pages;
        t->highest += pages * PAGE_SIZE;
    } else {
        // thread is trying to free memory
        // this is slightly tricky because we cant assume the caller page-aligned everything
        uintptr_t ptr = brk - (pages * PAGE_SIZE);
        if(delta % PAGE_SIZE) {
            pages--;
            ptr += PAGE_SIZE;
        }

        vmmFree(ptr, pages);
        t->pages -= pages;
        p->pages -= pages;
        t->highest -= pages * PAGE_SIZE;
    }

    return (void *) brk;
}