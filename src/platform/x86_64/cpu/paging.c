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

    pml4[0] = (uint64_t)pdp | PT_PAGE_PRESENT | PT_PAGE_RW | PT_PAGE_USER;
    
    uint64_t addr = 0;
    uint64_t *pd;
    for(int i = 0; i < IDENTITY_MAP_GBS; i++) {
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

    // now map memory at a high address
    uintptr_t v = KERNEL_MMIO_BASE;
    uintptr_t p = 0;
    
    for(size_t i = 0; i < (KERNEL_MMIO_GBS << 18); i++) {
        platformMapPage(v, p, PLATFORM_PAGE_PRESENT | PLATFORM_PAGE_WRITE);
        v += PAGE_SIZE;
        p += PAGE_SIZE;
    }

    ttyRemapFramebuffer();
    KDEBUG("kernel paging structures created, identity mapped %d GiB\n", IDENTITY_MAP_GBS);
    kernelPagingRoot = pml4;
    return 0;
}

/* platformGetPagingRoot(): returns a pointer to the base paging structure
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

    return memcpy((void *)ptr, kernelPagingRoot, PAGE_SIZE);
}

/* platformGetPage(): returns the physical address and flags of a logical address
 * params: flags - pointer to where to store the flags
 * params: addr - logical address
 * returns: physical address corresponding to logical address
 */

uintptr_t platformGetPage(int *flags, uintptr_t addr) {
    uint64_t highestIdentityAddress = ((uint64_t)IDENTITY_MAP_GBS << 30) - 1; // GiB to bytes
    if(addr <= highestIdentityAddress) {
        *flags = PLATFORM_PAGE_PRESENT | PLATFORM_PAGE_WRITE | PLATFORM_PAGE_EXEC;
        return addr;
    }

    *flags = 0;
    int pml4Index = (addr >> 39) & 511; // 512 GiB * 512 = some massive number
    int pdpIndex = (addr >> 30) & 511;  // 1 GiB * 512 = 512 GiB per PDP
    int pdIndex = (addr >> 21) & 511;   // 2 MiB * 512 = 1 GiB per PD
    int ptIndex = (addr >> 12) & 511;   // 4 KiB * 512 = 2 MiB per PT
    uintptr_t offset = addr & (PAGE_SIZE-1);

    // TODO: account for inconsistencies between virtual and physical addresses here
    uint64_t *pml4 = (uint64_t *)readCR3();
    uint64_t pml4Entry = pml4[pml4Index];
    if(!pml4Entry & PT_PAGE_PRESENT) {
        return 0;
    }

    uint64_t *pdp = (uint64_t *)(pml4Entry & ~(PAGE_SIZE-1));
    uint64_t pdpEntry = pdp[pdpIndex];
    if(!pdpEntry & PT_PAGE_PRESENT) {
        return 0;
    }

    uint64_t *pd = (uint64_t *)(pdpEntry & ~(PAGE_SIZE-1));
    uint64_t pdEntry = pd[pdIndex];
    if(!pdEntry & PT_PAGE_PRESENT) {
        return 0;
    }

    uint64_t *pt = (uint64_t *)(pdEntry & ~(PAGE_SIZE-1));
    uint64_t ptEntry = pt[ptIndex];
    
    if(ptEntry & PT_PAGE_PRESENT) *flags |= PLATFORM_PAGE_PRESENT;
    else if(ptEntry) *flags |= PLATFORM_PAGE_SWAP;  // not present in main memory but non-zero

    if(ptEntry & PT_PAGE_RW) *flags |= PLATFORM_PAGE_WRITE;
    if(ptEntry & PT_PAGE_USER) *flags |= PLATFORM_PAGE_USER;
    if(!(ptEntry & PT_PAGE_NXE)) *flags |= PLATFORM_PAGE_EXEC;
    if(ptEntry & PT_PAGE_NO_CACHE) *flags |= PLATFORM_PAGE_NO_CACHE;
    
    return (ptEntry & ~(PAGE_SIZE-1)) | offset;
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

    uint64_t *pml4 = (uint64_t *)readCR3();
    uint64_t pml4Entry = pml4[pml4Index];
    if(!pml4Entry & PT_PAGE_PRESENT) {
        pml4Entry = pmmAllocate();
        if(!pml4Entry) {
            KERROR("platformMapPage: map 0x%08X to 0x%08X\n", physical, logical);
            KERROR("failed to allocate memory for page directory pointer\n");
            return 0;
        }

        memset((void *)pml4Entry, 0, PAGE_SIZE);
        pml4[pml4Index] = pml4Entry | PT_PAGE_PRESENT | PT_PAGE_RW | PT_PAGE_USER;
    }

    uint64_t *pdp = (uint64_t *)(pml4Entry & ~(PAGE_SIZE-1));
    uint64_t pdpEntry = pdp[pdpIndex];
    if(!pdpEntry & PT_PAGE_PRESENT) {
        pdpEntry = pmmAllocate();
        if(!pdpEntry) {
            KERROR("platformMapPage: map 0x%08X to 0x%08X\n", physical, logical);
            KERROR("failed to allocate memory for page directory\n");
            return 0;
        }

        memset((void *)pdpEntry, 0, PAGE_SIZE);
        pdp[pdpIndex] = pdpEntry | PT_PAGE_PRESENT | PT_PAGE_RW | PT_PAGE_USER;
    }

    uint64_t *pd = (uint64_t *)(pdpEntry & ~(PAGE_SIZE-1));
    uint64_t pdEntry = pd[pdIndex];
    if(!pdEntry & PT_PAGE_PRESENT) {
        pdEntry = pmmAllocate();
        if(!pdEntry) {
            KERROR("platformMapPage: map 0x%08X to 0x%08X\n", physical, logical);
            KERROR("failed to allocate memory for page table\n");
            return 0;
        }

        memset((void *)pdEntry, 0, PAGE_SIZE);
        pd[pdIndex] = pdEntry | PT_PAGE_PRESENT | PT_PAGE_RW | PT_PAGE_USER;
    }

    uint64_t *pt = (uint64_t *)(pdEntry & ~(PAGE_SIZE-1));
    uint64_t parsedFlags = 0;

    if(flags & PLATFORM_PAGE_PRESENT) parsedFlags |= PT_PAGE_PRESENT;
    if(flags & PLATFORM_PAGE_WRITE) parsedFlags |= PT_PAGE_RW;
    if(flags & PLATFORM_PAGE_USER) parsedFlags |= PT_PAGE_USER;
    if(!flags & PLATFORM_PAGE_EXEC) parsedFlags |= PT_PAGE_NXE;
    if(flags & PLATFORM_PAGE_NO_CACHE) parsedFlags |= PT_PAGE_NO_CACHE | PT_PAGE_WRITE_THROUGH;

    pt[ptIndex] = physical | parsedFlags;
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
