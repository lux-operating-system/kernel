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

}