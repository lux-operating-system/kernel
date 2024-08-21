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
#include <kernel/logger.h>

static PhysicalMemoryStatus status;
static uint8_t *pmmBitmap;
static size_t pmmBitmapSize;

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

/* pmmInitMark(): marks pages as free or used without checking their status
 * this is necessary during the startup process while parsing the memory map 
 * params: phys - physical address
 * params: use - whether the page is used
 * returns: nothing
 */

static void pmmInitMark(uintptr_t phys, bool use) {
    uintptr_t page = phys / PAGE_SIZE;
    uintptr_t byte = page / 8;
    uintptr_t bit = page % 8;

    if(use) {
        pmmBitmap[byte] |= (1 << bit);
        status.reservedPages++;
    } else {
        pmmBitmap[byte] &= ~(1 << bit);
        status.usablePages++;
    }
}

static void pmmInitMarkContiguous(uintptr_t phys, size_t count, bool use) {
    for(size_t i = 0; i < count; i++) {
        pmmInitMark(phys, use);
        phys += PAGE_SIZE;
    }
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

    pmmBitmapSize = (status.highestPage + 7) / 8;

    // reset the bitmap reserving everything, and then mark the RAM regions as free later
    memset(pmmBitmap, 0xFF, pmmBitmapSize);
    MemoryMap *mmap = (MemoryMap *)boot->memoryMap;

    //status.usedPages = status.highestPage;

    KDEBUG("system memory map:\n");

    const char *memTypes[] = {
        "RAM",
        "reserved",
        "ACPI reclaimable",
        "ACPI NVS",
        "bad memory"
    };

    for(int i = 0; i < boot->memoryMapSize; i++) {
        KDEBUG(" %016X - %016X - %s\n", mmap[i].base, mmap[i].base+mmap[i].len-1, 
            mmap[i].type <= MEMORY_TYPE_BAD ? memTypes[(mmap[i].type%5) - 1] : "undefined");

        // the system doesn't have to implement ACPI 3.0 for us to check for this
        // the boot loader appends this flag on pre-ACPI 3.0 systems
        if(mmap[i].acpiAttributes & MEMORY_ATTRIBUTES_VALID) {
            switch(mmap[i].type) {
            case MEMORY_TYPE_USABLE:
                // don't round up here in case of a hypothetical scenario where
                // there is a boundary between usable and unusable memory on a
                // non-page-aligned boundary
                // this is very unlikely but keeps us safe just in case
                //status.usablePages += mmap[i].len / PAGE_SIZE;
                pmmInitMarkContiguous(mmap[i].base, mmap[i].len / PAGE_SIZE, false);

                if((mmap[i].base + mmap[i].len) > status.highestUsableAddress) {
                    status.highestUsableAddress = mmap[i].base + mmap[i].len - 1;
                }
                break;
            case MEMORY_TYPE_RESERVED:
            case MEMORY_TYPE_ACPI_RECLAIMABLE:
            case MEMORY_TYPE_ACPI_NVS:
            case MEMORY_TYPE_BAD:
            default:
                pmmInitMarkContiguous(mmap[i].base, (mmap[i].len + PAGE_SIZE - 1) / PAGE_SIZE, true);
            }
        }
    }

    // now reserve all the kernel's memory including ramdisks, modules
    // this is reserving until the end of the pmm bitmap
    uintptr_t pmmBitmapEnd = (uintptr_t)pmmBitmap + pmmBitmapSize + PAGE_SIZE - 1;
    size_t kernelPages = pmmBitmapEnd / PAGE_SIZE;
    pmmMarkContiguous(0, kernelPages, true);

    status.lowestUsableAddress = (uintptr_t)(pmmBitmap + pmmBitmapSize + PAGE_SIZE) & ~(PAGE_SIZE-1);

    KDEBUG("highest kernel address is 0x%08X\n", boot->kernelHighestAddress);
    KDEBUG("highest physical address is 0x%08X\n", boot->highestPhysicalAddress);
    KDEBUG("lowest usable address is 0x%08X\n", status.lowestUsableAddress);
    KDEBUG("highest usable address is 0x%08X\n", status.highestUsableAddress);

    KDEBUG("bitmap size = %d pages (%d KiB)\n", (pmmBitmapSize+PAGE_SIZE-1)/PAGE_SIZE, pmmBitmapSize/1024);
    KDEBUG("total usable memory = %d pages (%d MiB)\n", status.usablePages, (status.usablePages * PAGE_SIZE) / 0x100000);
    KDEBUG("kernel-reserved memory = %d pages (%d MiB)\n", status.usedPages, (status.usedPages * PAGE_SIZE) / 0x100000);
    KDEBUG("hardware-reserved memory = %d pages (%d MiB)\n", status.reservedPages, (status.reservedPages * PAGE_SIZE) / 0x100000);
}

/* pmmStatus(): returns the physical memory manager's status
 * params: dst - destination structure to copy the status to
 * returns: nothing
 */

void pmmStatus(PhysicalMemoryStatus *dst) {
    memcpy(dst, &status, sizeof(PhysicalMemoryStatus));
}

/* pmmIsUsed(): returns whether a page is used
 * params: phys - physical address
 * returns: true/false
 */

bool pmmIsUsed(uintptr_t phys) {
    // return true automatically for all unusable pages
    if(phys >= status.highestUsableAddress) return true;

    uintptr_t page = phys / PAGE_SIZE;
    uintptr_t byte = page / 8;
    uintptr_t bit = page % 8;

    return ((pmmBitmap[byte] >> bit) & 1);
}

/* pmmAllocate(): allocates one page
 * params: none
 * returns: physical address of the page allocated, zero on fail
 */

uintptr_t pmmAllocate(void) {
    uintptr_t addr;

    for(addr = status.lowestUsableAddress; addr < status.highestUsableAddress; addr += PAGE_SIZE) {
        if(!pmmIsUsed(addr)) {
            pmmMark(addr, true);
            //KDEBUG("allocated physical page at 0x%08X, %d pages in use\n", addr, status.usedPages);
            return addr;
        }
    }

    return 0;
}

/* pmmFree(): frees one page
 * params: phys - physical address of the page
 * returns: zero on success
 */

int pmmFree(uintptr_t phys) {
    if(phys <= status.lowestUsableAddress || phys >= status.highestUsableAddress) return -1;
    //KDEBUG("freeing memory at 0x%08X, %d pages in use\n", phys, status.usedPages);
    return pmmMark(phys, false);
}
