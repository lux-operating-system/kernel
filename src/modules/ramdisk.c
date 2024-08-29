/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

#include <stddef.h>
#include <kernel/modules.h>
#include <kernel/boot.h>
#include <kernel/logger.h>

static uint8_t *ramdisk;
static uint64_t ramdiskSize;

/* ramdiskInit(): initializes the ramdisk
 * params: boot - boot information structure
 * returns: nothing
 */

void ramdiskInit(KernelBootInfo *boot) {
    if(boot->ramdisk && boot->ramdiskSize) {
        KDEBUG("ramdisk is at 0x%08X\n", boot->ramdisk);
        KDEBUG("ramdisk size is %d KiB\n", boot->ramdiskSize/1024);

        ramdisk = (uint8_t *)boot->ramdisk;
        ramdiskSize = boot->ramdiskSize;
    } else {
        ramdisk = NULL;
        ramdiskSize = 0;
    }
}