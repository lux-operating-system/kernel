/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

/* Memory-Mapped I/O for Device Drivers */

#define MMIO_R          0x01    // read perms
#define MMIO_W          0x02    // write perms
#define MMIO_X          0x04    // execute perms
#define MMIO_CD         0x08    // cache disable
#define MMIO_ENABLE     0x80    // create mapping; clear to unmap

#include <kernel/logger.h>
#include <kernel/sched.h>
#include <kernel/memory.h>
#include <platform/platform.h>
#include <platform/mmap.h>
#include <sys/types.h>

/* mmio(): creates or deletes a memory mapping
 * params: t - calling thread
 * params: addr - physical address for creation, virtual address for deletion
 * params: count - number of bytes to be mapped/unmapped
 * params: flags - memory flags
 * returns: creation: virtual address on success, zero on fail
 * returns: deletion: zero on success, virtual address on fail
 */

uintptr_t mmio(Thread *t, uintptr_t addr, off_t count, int flags) {
    Process *p = getProcess(t->pid);
    if(!p) return 0;

    // only root can do this
    if(p->user) return 0;

    off_t offset = addr & (PAGE_SIZE-1);
    size_t pageCount = (count + PAGE_SIZE - 1) / PAGE_SIZE;
    if(addr & ~(PAGE_SIZE-1)) pageCount++;

    if(flags & MMIO_ENABLE) {
        // creating a memory mapping
        int pageFlags = PLATFORM_PAGE_PRESENT | PLATFORM_PAGE_USER;
        if(flags & MMIO_W) pageFlags |= PLATFORM_PAGE_WRITE;
        if(flags & MMIO_X) pageFlags |= PLATFORM_PAGE_EXEC;

        uintptr_t virt = vmmAllocate(USER_MMIO_BASE, USER_LIMIT_ADDRESS, pageCount, VMM_USER);
        if(!virt) return 0;

        for(int i = 0; i < pageCount; i++)
            platformMapPage(virt + (i*PAGE_SIZE), addr + (i*PAGE_SIZE), pageFlags);

        KDEBUG("mapped %d pages at physical addr 0x%X for tid %d\n", pageCount, addr, t->tid);
        return virt | offset;
    } else {
        // deleting a memory mapping
        if(addr < USER_MMIO_BASE) return addr;

        for(int i = 0; i < pageCount; i++)
            platformMapPage(addr + (i * PAGE_SIZE), 0, 0);

        KDEBUG("unmapped %d pages at virtual address 0x%X for tid %d\n", pageCount, addr, t->tid);
        return 0;
    }
}