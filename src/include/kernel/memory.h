/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>
#include <kernel/boot.h>
#include <kernel/sched.h>
#include <kernel/servers.h>

/* this must be defined on a platform-specific basis */
/* it defines the page size and other necessary attributes for paging */
#include <platform/mmap.h>

#define PMM_CONTIGUOUS_LOW      0x01

// these flags control allocated memory
#define VMM_USER                0x01        // kernel-user toggle
#define VMM_EXEC                0x02
#define VMM_WRITE               0x04
#define VMM_NO_CACHE            0x08

// these flags are used as platform-independent status codes after page faults
#define VMM_PAGE_FAULT_PRESENT  0x01        // caused by a present page
#define VMM_PAGE_FAULT_USER     0x02        // caused by a user process
#define VMM_PAGE_FAULT_WRITE    0x04        // caused by a write operation
#define VMM_PAGE_FAULT_FETCH    0x08        // caused by instruction fetch

// these values are used as "magic" addresses for pages marked as swap (i.e. not-present)
// they are all 2 MiB-aligned for compatibility across architectures
// they essentially indicate to the virtual memory manager how to handle a page fault
#define VMM_PAGE_SWAP_MASK      0xE00000
#define VMM_PAGE_SWAP           0x200000    // swap from disk
#define VMM_PAGE_ALLOCATE       0x400000    // allocate physical memory

// TODO: adjust bit masks and shifting here when implementing true swapping

// protection and flags for memory-mapped files
#define PROT_READ               0x01
#define PROT_WRITE              0x02
#define PROT_EXEC               0x04
#define PROT_NONE               0x00

#define MAP_SHARED              0x01
#define MAP_PRIVATE             0x02
#define MAP_FIXED               0x04

#define MS_ASYNC                0x01
#define MS_SYNC                 0x02
#define MS_INVALIDATE           0x04

typedef struct {
    uint64_t highestPhysicalAddress;
    uint64_t lowestUsableAddress;
    uint64_t highestUsableAddress;
    size_t highestPage;
    size_t usablePages, usedPages;
    size_t reservedPages;
} PhysicalMemoryStatus;

typedef struct {
    uint64_t usedPages, usedBytes;
} KernelHeapStatus;

typedef struct {
    int fd, prot, flags;
    pid_t pid, tid;     // original owner
    bool device;
    size_t length;
    off_t offset;
} MmapHeader;

struct MmapSyscallParams {
    // probably the only syscall whose params will be passed in memory because
    // there's just too many
    void *addr;
    size_t len;
    int prot;
    int flags;
    int fd;
    off_t off;
};

void pmmInit(KernelBootInfo *);
void pmmStatus(PhysicalMemoryStatus *);
uintptr_t pmmAllocate(void);
uintptr_t pmmAllocateContiguous(size_t, int);
int pmmFree(uintptr_t);
int pmmFreeContiguous(uintptr_t, size_t);

void vmmInit();
uintptr_t vmmAllocate(uintptr_t, uintptr_t, size_t, int);
int vmmFree(uintptr_t, size_t);
int vmmPageFault(uintptr_t, int);       // the platform-specific page fault handler must call this
uintptr_t vmmMMIO(uintptr_t, bool);
int vmmPageStatus(uintptr_t, uintptr_t *);
uintptr_t vmmSetFlags(uintptr_t, size_t, int);

void *sbrk(Thread *, intptr_t);

uintptr_t mmio(Thread *, uintptr_t, off_t, int);
uintptr_t pcontig(Thread *, uintptr_t, off_t, int);
uintptr_t vtop(Thread *, uintptr_t);

void *mmap(Thread *, uint64_t, void *, size_t, int, int, int, off_t);
int munmap(Thread *, void *, size_t);
int msync(Thread *, uint64_t, void *, size_t, int);

void mmapHandle(MmapCommand *, SyscallRequest *);
