/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Platform-Specific Code for x86_64
 */

#include <string.h>
#include <stdint.h>
#include <platform/x86_64.h>
#include <platform/platform.h>
#include <kernel/logger.h>
#include <kernel/memory.h>
#include <kernel/tty.h>

static uint64_t *kernelPagingRoot;  // pml4 -- PHYSICAL ADDRESS

/* platformPagingSetup(): sets up the kernel's paging structures
 * this is called by the virtual memory manager early in the boot process
 */

int platformPagingSetup() {
    // check for PAE/NX
    CPUIDRegisters regs;
    memset(&regs, 0, sizeof(CPUIDRegisters));
    readCPUID(0x80000001, &regs);
    if(!(regs.edx & (1 << 20))) {
        KERROR("CPU doesn't support PAE/NX\n");
        while(1);
    }

    // enable no-execute pages
    writeMSR(MSR_EFER, readMSR(MSR_EFER) | MSR_EFER_NX_ENABLE);

    // map physical memory into the higher half
    uint64_t *pml4 = (uint64_t *)pmmAllocate();     // 512 GiB per entry
    uint64_t *pdp = (uint64_t *)pmmAllocate();      // 1 GiB per entry

    if(!pml4 || !pdp) {
        KERROR("unable to allocate memory for paging root structs\n");
        return -1;
    }

    memset(pml4, 0, PAGE_SIZE);
    memset(pdp, 0, PAGE_SIZE);

    //pml4[0] = (uint64_t)pdp | PT_PAGE_PRESENT | PT_PAGE_RW | PT_PAGE_USER;
    pml4[256] = (uint64_t)pdp | PT_PAGE_PRESENT | PT_PAGE_RW;
    
    uint64_t addr = 0;
    uint64_t *pd;
    for(int i = 0; i < KERNEL_BASE_MAPPED; i++) {
        pd = (uint64_t *)pmmAllocate();
        if(!pd) {
            KERROR("unable to allocate memory for page directory %d\n", i);
            return -1;
        }

        pdp[i] = (uint64_t)pd | PT_PAGE_PRESENT | PT_PAGE_RW | PT_PAGE_USER;

        for(int j = 0; j < 512; j++) {
            pd[j] = addr | PT_PAGE_PRESENT | PT_PAGE_RW | PT_PAGE_SIZE_EXTENSION;
            addr += 0x200000;
        }
    }

    // load the new paging roots
    writeCR3((uint64_t)pml4);

    ttyRemapFramebuffer();
    KDEBUG("kernel paging structures created, mapped %d GiB at 0x%X\n", KERNEL_BASE_MAPPED, KERNEL_MMIO_BASE);
    kernelPagingRoot = pml4;
    return 0;
}

/* platformGetPagingRoot(): returns a PHYSICAL pointer to the base paging structure
 * this is of type void * for platform-independence */

void *platformGetPagingRoot() {
    return kernelPagingRoot;
}

/* platformCloneKernelSpace(): creates a clone of the kernel's paging root
 * params: none
 * params: pointer to the new paging root, NULL on failure
 */

void *platformCloneKernelSpace() {
    uintptr_t ptr = pmmAllocate();
    if(!ptr) return NULL;

    return memcpy((void *)vmmMMIO(ptr, true), (const void *)vmmMMIO((uintptr_t)kernelPagingRoot, true), PAGE_SIZE);
}

/* platformGetPage(): returns the physical address and flags of a logical address
 * params: flags - pointer to where to store the flags
 * params: addr - logical address
 * returns: physical address corresponding to logical address
 */

uintptr_t platformGetPage(int *flags, uintptr_t addr) {
    /*uint64_t highestIdentityAddress = ((uint64_t)IDENTITY_MAP_GBS << 30) - 1; // GiB to bytes
    if(addr <= highestIdentityAddress) {
        *flags = PLATFORM_PAGE_PRESENT | PLATFORM_PAGE_WRITE | PLATFORM_PAGE_EXEC;
        return addr;
    }*/

    if(addr >= KERNEL_BASE_ADDRESS && addr <= KERNEL_BASE_END) {
        *flags = PLATFORM_PAGE_PRESENT | PLATFORM_PAGE_WRITE | PLATFORM_PAGE_EXEC;
        return addr - KERNEL_BASE_ADDRESS;
    }

    *flags = 0;
    int pml4Index = (addr >> 39) & 511; // 512 GiB * 512 = some massive number
    int pdpIndex = (addr >> 30) & 511;  // 1 GiB * 512 = 512 GiB per PDP
    int pdIndex = (addr >> 21) & 511;   // 2 MiB * 512 = 1 GiB per PD
    int ptIndex = (addr >> 12) & 511;   // 4 KiB * 512 = 2 MiB per PT
    uintptr_t offset = addr & (PAGE_SIZE-1);

    // TODO: account for inconsistencies between virtual and physical addresses here
    uint64_t *pml4 = (uint64_t *)vmmMMIO(readCR3() & ~(PAGE_SIZE-1), true);
    uint64_t pml4Entry = pml4[pml4Index];
    if(!(pml4Entry & PT_PAGE_PRESENT)) return 0;

    uint64_t *pdp = (uint64_t *)vmmMMIO((pml4Entry & ~(PAGE_SIZE-1)), true);
    uint64_t pdpEntry = pdp[pdpIndex];
    if(!(pdpEntry & PT_PAGE_PRESENT)) return 0;

    uint64_t *pd = (uint64_t *)vmmMMIO((pdpEntry & ~(PAGE_SIZE-1)), true);
    uint64_t pdEntry = pd[pdIndex];
    if(!(pdEntry & PT_PAGE_PRESENT)) return 0;

    uint64_t *pt = (uint64_t *)vmmMMIO((pdEntry & ~(PAGE_SIZE-1)), true);
    uint64_t ptEntry = pt[ptIndex];
    
    if(ptEntry & PT_PAGE_PRESENT) *flags |= PLATFORM_PAGE_PRESENT;
    else if(ptEntry) *flags |= PLATFORM_PAGE_SWAP;  // not present in main memory but non-zero

    if(ptEntry & PT_PAGE_RW) *flags |= PLATFORM_PAGE_WRITE;
    if(ptEntry & PT_PAGE_USER) *flags |= PLATFORM_PAGE_USER;
    if(!(ptEntry & PT_PAGE_NXE)) *flags |= PLATFORM_PAGE_EXEC;
    if(ptEntry & PT_PAGE_NO_CACHE) *flags |= PLATFORM_PAGE_NO_CACHE;
    
    return (ptEntry & ~(PAGE_SIZE-1) & ~(PT_PAGE_NXE)) | offset;
}

/* platformMapPage(): maps a physical address to a logical address
 * params: logical - logical address, page-aligned
 * params: physical - physical address, page-aligned
 * params: flags - page flags requested
 * returns: logical address on success, 0 on failure
 */

uintptr_t platformMapPage(uintptr_t logical, uintptr_t physical, int flags) {
    // force page-alignment
    logical &= ~(PAGE_SIZE-1);
    physical &= ~(PAGE_SIZE-1);

    int pml4Index = (logical >> 39) & 511; // 512 GiB * 512 = some massive number
    int pdpIndex = (logical >> 30) & 511;  // 1 GiB * 512 = 512 GiB per PDP
    int pdIndex = (logical >> 21) & 511;   // 2 MiB * 512 = 1 GiB per PD
    int ptIndex = (logical >> 12) & 511;   // 4 KiB * 512 = 2 MiB per PT

    uint64_t *pml4 = (uint64_t *)vmmMMIO(readCR3(), true);
    uint64_t pml4Entry = pml4[pml4Index];
    if(!pml4Entry & PT_PAGE_PRESENT) {
        pml4Entry = pmmAllocate();
        if(!pml4Entry) {
            KERROR("platformMapPage: map 0x%08X to 0x%08X\n", physical, logical);
            KERROR("failed to allocate memory for page directory pointer\n");
            return 0;
        }

        memset((void *)vmmMMIO(pml4Entry, true), 0, PAGE_SIZE);
        pml4[pml4Index] = pml4Entry | PT_PAGE_PRESENT | PT_PAGE_RW | PT_PAGE_USER;
    }

    uint64_t *pdp = (uint64_t *)vmmMMIO((pml4Entry & ~(PAGE_SIZE-1)), true);
    uint64_t pdpEntry = pdp[pdpIndex];
    if(!pdpEntry & PT_PAGE_PRESENT) {
        pdpEntry = pmmAllocate();
        if(!pdpEntry) {
            KERROR("platformMapPage: map 0x%08X to 0x%08X\n", physical, logical);
            KERROR("failed to allocate memory for page directory\n");
            return 0;
        }

        memset((void *)vmmMMIO(pdpEntry, true), 0, PAGE_SIZE);
        pdp[pdpIndex] = pdpEntry | PT_PAGE_PRESENT | PT_PAGE_RW | PT_PAGE_USER;
    }

    uint64_t *pd = (uint64_t *)vmmMMIO((pdpEntry & ~(PAGE_SIZE-1)), true);
    uint64_t pdEntry = pd[pdIndex];
    if(!pdEntry & PT_PAGE_PRESENT) {
        pdEntry = pmmAllocate();
        if(!pdEntry) {
            KERROR("platformMapPage: map 0x%08X to 0x%08X\n", physical, logical);
            KERROR("failed to allocate memory for page table\n");
            return 0;
        }

        memset((void *)vmmMMIO(pdEntry, true), 0, PAGE_SIZE);
        pd[pdIndex] = pdEntry | PT_PAGE_PRESENT | PT_PAGE_RW | PT_PAGE_USER;
    }

    uint64_t *pt = (uint64_t *)vmmMMIO((pdEntry & ~(PAGE_SIZE-1)), true);
    uint64_t parsedFlags = 0;

    if(flags & PLATFORM_PAGE_PRESENT) parsedFlags |= PT_PAGE_PRESENT;
    if(flags & PLATFORM_PAGE_WRITE) parsedFlags |= PT_PAGE_RW;
    if(flags & PLATFORM_PAGE_USER) parsedFlags |= PT_PAGE_USER;
    if(!(flags & PLATFORM_PAGE_EXEC)) parsedFlags |= PT_PAGE_NXE;
    if(flags & PLATFORM_PAGE_NO_CACHE) parsedFlags |= PT_PAGE_NO_CACHE | PT_PAGE_WRITE_THROUGH;

    pt[ptIndex] = physical | parsedFlags;

    // maintain canonical addresses
    if(logical & ((uint64_t)1 << 47)) return logical | 0xFFF0000000000000;
    return logical;
}

/* platformUnmapPage(): unmaps a physical address from a logical address 
 * params: addr - logical address
 * returns: 0 on success
 */

int platformUnmapPage(uintptr_t addr) {
    // essentially just map it to physical address zero with flags set to zero
    // this will allow us to differentiate between truly unused pages and absent
    // but swappable pages in secondary storage
    addr &= ~(PAGE_SIZE-1);
    return !(platformMapPage(addr, 0, 0) == addr);
}

/* clonePagingLayer(): helper recursive function that clones a single paging layer
 * this works for PDPs, PDs, and PTs
 * params: ptr - physical pointer to the paging structure
 * params: layer - 0 for PDPs, 1 for PDs, and 2 for PTs
 * returns: physical pointer to the clone, zero on fail
 */

uint64_t clonePagingLayer(uint64_t ptr, int layer) {
    if(!ptr || layer < 0 || layer > 2) return 0;

    uint64_t *parent = (uint64_t *)vmmMMIO(ptr & ~(PAGE_SIZE-1), true);
    uint64_t cloneBase = pmmAllocate();
    if(!cloneBase) return 0;
    uint64_t *clone = (uint64_t *)vmmMMIO(cloneBase, true);

    uint64_t newPhys, oldPhys;

    for(int i = 0; i < 512; i++) {
        if(parent[i] & PT_PAGE_PRESENT) {
            // are we working with the PT?
            if(layer == 2) {
                newPhys = pmmAllocate();
                if(!newPhys) return 0;

                oldPhys = parent[i] & ~((PAGE_SIZE-1) | PT_PAGE_NXE);
                memcpy((void *)vmmMMIO(newPhys, true), (const void *)vmmMMIO(oldPhys, true), PAGE_SIZE);

                clone[i] = newPhys | (parent[i] & ((uint64_t)PT_PAGE_LOW_FLAGS | PT_PAGE_NXE));   // copy the parent's permissions
            } else {
                // here we're working with either the PDP or PD that is also present
                newPhys = pmmAllocate();
                if(!newPhys) return 0;

                oldPhys = parent[i] & ~(PAGE_SIZE-1);
                clone[i] = clonePagingLayer(oldPhys, layer+1);
                clone[i] |= parent[i] & PT_PAGE_LOW_FLAGS;  // copy permissions again
            }
        } else {
            clone[i] = 0;
        }
    }

    return cloneBase;
}

/* platformCloneUserSpace(): deep clones the user address space
 * params: parent - physical pointer to the PML4 of the parent
 * returns: physical pointer to the child PML4, NULL on fail
 */

void *platformCloneUserSpace(uintptr_t parent) {
    uint64_t base = pmmAllocate();
    if(!base) return NULL;

    uint64_t *newPML4 = (uint64_t *)vmmMMIO(base, true);
    uint64_t *oldPML4 = (uint64_t *)vmmMMIO(parent & ~(PAGE_SIZE-1), true);

    for(int i = 0; i < 256; i++) {
        uint64_t ptr = oldPML4[i] & ~(PAGE_SIZE-1);
        uint64_t flags = oldPML4[i] & PT_PAGE_LOW_FLAGS;
        if((flags & PT_PAGE_PRESENT) && ptr) {
            newPML4[i] = clonePagingLayer(ptr, 0) | flags;
        } else {
            newPML4[i] = 0;
        }
    }

    for(int i = 256; i < 512; i++) {
        newPML4[i] = oldPML4[i];    // the kernel is in every address space
    }

    //KDEBUG("cloned PML4 at 0x%08X into 0x%08X\n", parent, base);

    return (void *) base;
}