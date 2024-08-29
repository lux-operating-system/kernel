/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

#include <stddef.h>
#include <string.h>
#include <kernel/modules.h>
#include <kernel/boot.h>
#include <kernel/logger.h>

static uint8_t *modules[MAX_MODULES];
static uint64_t moduleSizes[MAX_MODULES];

/* modulesInit(): initializes the boot modules
 * params: boot - boot information structure
 * returns: nothing
 */

void modulesInit(KernelBootInfo *boot) {
    memset(modules, 0, MAX_MODULES * sizeof(uint8_t));
    memset(moduleSizes, 0, MAX_MODULES * sizeof(uint64_t));

    if(boot->moduleCount) {
        KDEBUG("enumerating boot modules...\n");

        for(int i = 0; i < boot->moduleCount; i++) {
            modules[i] = (uint8_t *)boot->modules[i];
            moduleSizes[i] = boot->moduleSizes[i];

            KDEBUG(" %d of %d: %s loaded at 0x%08X\n", i+1, boot->moduleCount, modules[i], modules[i]);
        }
    }
}
