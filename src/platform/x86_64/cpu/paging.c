/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Platform-Specific Code for x86_64
 */

#include <string.h>
#include <platform/x86_64.h>
#include <platform/platform.h>
#include <kernel/logger.h>
#include <kernel/memory.h>

static uint64_t *kernelPagingRoot;  // pml4

/* platformPagingSetup(): sets up the kernel's paging structures
 * this is called by the virtual memory manager early in the boot process
 */

int platformPagingSetup() {
    // identity map the lower memory
    // this is already true but we need to replace the paging structs built
    // by the boot loader
    uint64_t *pml4 = (uint64_t *)pmmAllocate();     // 512 GiB per entry
    uint64_t *pdp = (uint64_t *)pmmAllocate();      // 1 GiB per entry

    if(!pml4 || !pdp) {
        KERROR("unable to allocate memory for paging root structs\n");
        return -1;
    }

    memset(pml4, 0, PAGE_SIZE);
    memset(pdp, 0, PAGE_SIZE);

    pml4[0] = (uint64_t)pdp | PT_PAGE_PRESENT | PT_PAGE_RW;
    
    uint64_t addr = 0;
    uint64_t *pd;
    for(int i = 0; i < IDENTITY_MAP_GBS; i++) {
        pd = (uint64_t *)pmmAllocate();
        if(!pd) {
            KERROR("unable to allocate memory for page directory %d\n", i);
            return -1;
        }

        pdp[i] = (uint64_t)pd | PT_PAGE_PRESENT | PT_PAGE_RW;

        for(int j = 0; j < 512; j++) {
            pd[j] = addr | PT_PAGE_PRESENT | PT_PAGE_RW | PT_PAGE_SIZE_EXTENSION;
            addr += 0x200000;
        }
    }

    // load the new paging roots
    writeCR3((uint64_t)pml4);
    KDEBUG("kernel paging structures created, identity mapped %d GiB\n", IDENTITY_MAP_GBS);
    kernelPagingRoot = pml4;
    return 0;
}

/* platformGetPagingRoot(): returns a pointer to the base paging structure
 * this is of type void * for platform-independence */

void *platformGetPagingRoot() {
    return kernelPagingRoot;
}
