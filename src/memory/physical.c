/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

/* Physical Memory Manager */

#include <stdbool.h>
#include <string.h>
#include <kernel/memory.h>
#include <kernel/boot.h>

static PhysicalMemoryStatus status;
uint8_t *pmmBitmap;

/* pmmMark(): marks a page as free or used
 * params: phys - physical address
 * params: use - whether the page is used
 * returns: 0 on success
 */

int pmmMark(uintptr_t phys, bool use) {
    uintptr_t page = phys / PAGE_SIZE;
    uintptr_t byte = page / 8;
    uintptr_t bit = page % 8;

    if(use) {
        if(pmmBitmap[byte] & (1 << bit)) {
            return -1;      // already marked as used
        } else {
            pmmBitmap[byte] |= (1 << bit);
            status.usedPages++;
        }
    } else {
        if(!(pmmBitmap[byte] & (1 << bit))) {
            return -1;      // already marked as free 
        } else {
            pmmBitmap[byte] &= ~(1 << bit);
            status.usedPages--;
        }
    }

    return 0;
}

/* pmmMarkContiguous(): marks contiguous pages as free or used
 * params: phys - physical address
 * params: count - number of pages
 * params: use - whether the page is used
 * returns: 0 on success
 */

int pmmMarkContiguous(uintptr_t phys, size_t count, bool use) {
    int status = 0;
    for(size_t i = 0; i < count; i++) {
        status |= pmmMark(phys, use);
        phys += PAGE_SIZE;
    }

    return status;
}

/* pmmInit(): this is called from platformMain() early in the boot process
 * params: boot - kernel boot information structure
 * returns: nothing
 */

void pmmInit(KernelBootInfo *boot) {
    memset(&status, 0, sizeof(PhysicalMemoryStatus));

    // TODO: change to moduleHighestAddress after boot modules are implemented
    pmmBitmap = (uint8_t *)boot->kernelHighestAddress;

    status.highestPhysicalAddress = boot->highestPhysicalAddress;
    status.highestPage = (status.highestPhysicalAddress + PAGE_SIZE - 1) / PAGE_SIZE;

    // reset the bitmap and then reserve the kernel space, IO registers, etc
    memset(pmmBitmap, 0, status.highestPage);
    MemoryMap *mmap = (MemoryMap *)boot->memoryMap;

    for(int i = 0; i < boot->memoryMapSize; i++) {
        // the system doesn't have to implement ACPI 3.0 for us to check for this
        // the boot loader appends this flag on pre-ACPI 3.0 systems
        if(mmap[i].acpiAttributes & MEMORY_ATTRIBUTES_VALID) {
            switch(mmap[i].type) {
            case MEMORY_TYPE_USABLE:
                // don't round up here in case of a hypothetical scenario where
                // there is a boundary between usable and unusable memory on a
                // non-page-aligned boundary
                // this is very unlikely but keeps us safe just in case
                status.usablePages += mmap[i].len / PAGE_SIZE;
                break;
            case MEMORY_TYPE_RESERVED:
            case MEMORY_TYPE_ACPI_RECLAIMABLE:
            case MEMORY_TYPE_ACPI_NVS:
            case MEMORY_TYPE_BAD:
            default:
                pmmMarkContiguous(mmap[i].base, (mmap[i].len + PAGE_SIZE - 1) / PAGE_SIZE, true);
            }
        }
    }

    // make usedPages track what's used out of actual usable memory
    // we need this because pmmMark() used in initialization alters usedPages
    status.reservedPages = status.usedPages;
    status.usedPages = 0;

    // now reserve all the kernel's memory including ramdisks, modules, etc
    // likewise, todo: replace this with moduleHighestAddress eventually
    size_t kernelPages = (boot->kernelHighestAddress + PAGE_SIZE - 1) / PAGE_SIZE;
    pmmMarkContiguous(0, kernelPages, true);
}
