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

/* vmmAllocate(): allocates virtual memory
 * params: base - base of logical address
 * params: limit - limit of logical address
 * params: count - number of pages to allocate
 * params: flags - attributes of the memory to be allocated
 * returns: pointer to the allocated memory, 0 on failure
 */

uintptr_t vmmAllocate(uintptr_t base, uintptr_t limit, size_t count, int flags) {
    // find free virtual memory
    base &= ~(PAGE_SIZE-1);
    limit &= ~(PAGE_SIZE-1);
    uintptr_t start = base;
    uintptr_t end = limit - (count * PAGE_SIZE);
    uintptr_t addr;
    
    // we are NOT setting the page-present flag here for performance reasons
    // true physical memory will only be allocated when the memory is used
    int platformFlags = 0;
    if(flags & VMM_USER) platformFlags |= PLATFORM_PAGE_USER;
    if(flags & VMM_WRITE) platformFlags |= PLATFORM_PAGE_WRITE;
    if(flags & VMM_EXEC) platformFlags |= PLATFORM_PAGE_EXEC;
    if(flags & VMM_NO_CACHE) platformFlags |= PLATFORM_PAGE_NO_CACHE;

    do {
        for(addr = start; addr < (start + (count*PAGE_SIZE)); addr += PAGE_SIZE) {
            if(vmmIsUsed(addr)) break;
        }

        if(addr >= (start + (count*PAGE_SIZE))) {
            for(size_t i = 0; i < count; i++) {
                if(!platformMapPage(start + (i*PAGE_SIZE), VMM_PAGE_ALLOCATE, platformFlags)) {
                    return 0;
                }
            }

            return start;
        }

        start += PAGE_SIZE;
    } while(start < end);

    return 0;
}

/* vmmFree(): frees virtual memory and associated physical memory/swap space
 * params: addr - logical address to be freed
 * params: count - page count
 * returns: 0 on success
 */

int vmmFree(uintptr_t addr, size_t count) {
    // force page alignment
    addr &= ~(PAGE_SIZE-1);


    int status = 0;
    int pageStatus;
    uintptr_t phys;

    for(size_t i = 0; i < count; i++) {
        pageStatus = vmmPageStatus(addr + (i * PAGE_SIZE), &phys);
        if(pageStatus & PLATFORM_PAGE_ERROR) {
            status |= 1;
        } else if(pageStatus & PLATFORM_PAGE_PRESENT) {
            status |= pmmFree(phys);
        } else if(pageStatus & PLATFORM_PAGE_SWAP) {
            // TODO: free swap space when swapping is implemented
        }

        // now free the virtual page itself
        status |= platformUnmapPage(addr + (i * PAGE_SIZE));
    }

    return status;
}

/* vmmPageFault(): platform-independent page fault handler
 * params: addr - logical address that caused the fault
 * params: access - access conditions that caused the fault
 * returns: 0 on success
 */

int vmmPageFault(uintptr_t addr, int access) {
    // determine the conditions that caused the fault
    if(access & VMM_PAGE_FAULT_PRESENT) {
        // page faults on a present page indicate privilege violations
        // this is an automatic fail
        KWARN("access violation at 0x%016X\n", addr);
        return -1;
    }

    // get the conditions of the page that caused the fault
    uintptr_t phys;
    int status = vmmPageStatus(addr & ~(PAGE_SIZE-1), &phys);
    //KDEBUG("physical: 0x%08X  status: 0x%02X\n", phys, status);

    // invalid page?
    if(status & PLATFORM_PAGE_ERROR) return -1;

    // no exec perms and attempt to fetch?
    if(!(status & PLATFORM_PAGE_EXEC) && (access & VMM_PAGE_FAULT_FETCH)) return -1;
    // user accessing kernel page?
    if(!(status & PLATFORM_PAGE_USER) && (access & VMM_PAGE_FAULT_USER)) return -1;
    // writing to read-only page?
    if(!(status & PLATFORM_PAGE_WRITE) && (access & VMM_PAGE_FAULT_WRITE)) return -1;

    // at this point we know there hasn't been any violations
    // so bring the page into memory if necessary
    int returnValue = -1;
    if(status & PLATFORM_PAGE_SWAP) {
        uint64_t swapFlags = phys & VMM_PAGE_SWAP_MASK;
        switch(swapFlags) {
        case VMM_PAGE_SWAP:
            KERROR("TODO: page swapping is not implemented yet; returning failure for now\n");
            break;
        case VMM_PAGE_ALLOCATE:
            /* here we need to allocate a physical page */
            phys = pmmAllocate();
            if(!phys) {
                KERROR("ran out of physical memory while handling page fault\n");
                break;
            }

            // map the physical page and return
            if(!platformMapPage(addr & ~(PAGE_SIZE-1), phys, status | PLATFORM_PAGE_PRESENT)) {
                KERROR("could not map physical page 0x%08X to logical 0x%08X\n", phys, addr & ~(PAGE_SIZE-1));
                break;
            }

            //KDEBUG("handled page fault; allocated physical 0x%08X to logical 0x%08X\n", phys, addr & ~(PAGE_SIZE-1));
            returnValue = 0;
            break;
        default:
            KERROR("undefined page table value 0x%016X\n", phys);
        }
    }

    return returnValue;
}

/* vmmMMIO(): requests an MMIO mapping 
 * params: phys - physical address
 * params: cache - cache enable
 * returns: pointer to virtual address, zero on fail
 */

uintptr_t vmmMMIO(uintptr_t phys, bool cache) {
    if(cache && (phys < KERNEL_MMIO_LIMIT)) {
        return phys + KERNEL_MMIO_BASE;
    } else {
        return 0;
    }
}

/* vmmSetFlags(): sets the flags for a series of pages
 * params: base - base address
 * params: count - number of pages
 * params: flags - page attributes to set
 * returns: base address
 */

uintptr_t vmmSetFlags(uintptr_t base, size_t count, int flags) {
    uintptr_t phys;
    int parsedFlags = PLATFORM_PAGE_PRESENT;
    if(flags & VMM_EXEC) parsedFlags |= PLATFORM_PAGE_EXEC;
    if(flags & VMM_USER) parsedFlags |= PLATFORM_PAGE_USER;
    if(flags & VMM_WRITE) parsedFlags |= PLATFORM_PAGE_WRITE;

    for(size_t i = 0; i < count; i++) {
        if(vmmPageStatus(base + (i*PAGE_SIZE), &phys) & PLATFORM_PAGE_PRESENT)
            platformMapPage(base + (i*PAGE_SIZE), phys, parsedFlags);
    }

    return base;
}