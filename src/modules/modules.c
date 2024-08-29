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

/* moduleCount(): returns the module count
 * params: none
 * returns: number of boot modules loaded
 */

int moduleCount() {
    int n = 0;
    for(int i = 0; i < MAX_MODULES; i++) {
        if(moduleSizes[i] && modules[i]) n++;
    }

    return n;
}

/* moduleFind(): finds a module by name
 * params: name - name of the module
 * returns: index to the model, -1 on fail
 */

int moduleFind(const char *name) {
    int c = moduleCount();

    for(int i = 0; i < c; i++) {
        if(modules[i] && !strcmp((const char *)modules[i], name)) {
            return i;
        }
    }

    return -1;
}

/* moduleQuery(): queries the size and existence of a module
 * params: name - name of the module
 * returns: size of the module in bytes, zero if it doesn't exist
 */

size_t moduleQuery(const char *name) {
    int i = moduleFind(name);
    if(i < 0) return 0;

    return (size_t) moduleSizes[i];
}

/* moduleLoad(): loads a module into memory 
 * params: buffer - buffer to load the module, at least size moduleQuery()
 * params: name - name of the module
 * returns: pointer to the buffer, NULL on failure
 */

void *moduleLoad(void *buffer, const char *name) {
    int i = moduleFind(name);
    if(i < 0) return NULL;

    uint8_t *ptr = modules[i];
    ptr += strlen((const char *)ptr) + 1;

    if(!moduleSizes[i]) return NULL;
    return memcpy(buffer, ptr, moduleSizes[i]);
}
