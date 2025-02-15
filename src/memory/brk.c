/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

/* Program Break Memory Manager */

#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <platform/platform.h>
#include <kernel/sched.h>
#include <kernel/memory.h>

/* sbrk(): changes the size of the program's data segment by moving the program break
 * params: t - calling thread
 * params: delta - number of bytes to increment or decrement
 * returns: pointer to previous program break on success, negative errno on fail
 */

void *sbrk(Thread *t, intptr_t delta) {
    intptr_t brk = t->highest;
    if(!delta) return (void *) brk;

    size_t pages;
    if(delta < 0) pages = ((-delta)+PAGE_SIZE-1) / PAGE_SIZE;
    else pages = (delta+PAGE_SIZE-1) / PAGE_SIZE;

    Process *p = getProcess(t->pid);
    if(!p) return (void *) -ESRCH;

    if(delta > 0) {
        // thread is trying to allocate memory
        // we will be optimistic here and allocate virtual memory only, making
        // the physical mm work only upon access
        uintptr_t ptr = vmmAllocate(brk, USER_LIMIT_ADDRESS, pages, VMM_USER | VMM_WRITE);
        if(!ptr) {
            return (void *) -ENOMEM;
        } else if(ptr != brk) {
            vmmFree(ptr, pages);
            return (void *) -ENOMEM;
        }

        memset((void *)ptr, 0, pages*PAGE_SIZE);

        t->pages += pages;
        p->pages += pages;
        t->highest += (pages * PAGE_SIZE);
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
        t->highest -= (pages * PAGE_SIZE);
    }

    return (void *) brk;
}