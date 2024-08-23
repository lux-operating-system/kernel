/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

#include <string.h>
#include <stdbool.h>
#include <platform/platform.h>
#include <kernel/memory.h>
#include <kernel/logger.h>

static KernelHeapStatus status;

/* vmmInit(): initializes the virtual memory manager */

void vmmInit() {
    if(platformPagingSetup()) {
        KERROR("failed to create paging structures; cannot initialize virtual memory manager\n");
        while(1);
    }

    memset(&status, 0, sizeof(KernelHeapStatus));
}

/* vmmPageStatus(): returns the status of a page
 * params: logical - logical address
 * params: phys - pointer to where to store physical address, NULL if undesired
 * returns: attributes of the page
 */

int vmmPageStatus(uintptr_t logical, uintptr_t *phys) {
    int s;
    uintptr_t p = platformGetPage(&s, logical);
    if(phys) *phys = p;
    return s;
}

/* vmmIsUsed(): returns if a logical address is used
 * params: addr - logical address
 * returns: true/false
 */

bool vmmIsUsed(uintptr_t addr) {
    int s = vmmPageStatus(addr, NULL);
    return (s & PLATFORM_PAGE_PRESENT) || (s & PLATFORM_PAGE_SWAP);
}

/* vmmAllocate(): allocates physical memory and maps to logical memory
 * params: base - base of logical address
 * params: count - number of pages to allocate
 * params: flags - attributes of the memory to be allocated
 * returns: pointer to the allocated memory, 0 on failure
 */

uintptr_t vmmAllocate(uintptr_t base, size_t count, int flags) {
    uintptr_t start = base;
    return 0;   // stub
}