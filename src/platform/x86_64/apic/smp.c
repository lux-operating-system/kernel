/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Platform-Specific Code for x86_64
 */

/* Symmetric Multiprocessing Implementation */

#include <stddef.h>
#include <platform/platform.h>
#include <platform/smp.h>
#include <kernel/logger.h>

static PlatformCPU *cpus = NULL;
static PlatformCPU *last = NULL;
static size_t cpuCount = 0;
static size_t runningCpuCount = 1;      // boot CPU

/* platformRegisterCPU(): registers a CPU so that the core OS code can know
 * params: cpu - pointer to a CPU structure (see smp.h for why the void type)
 * returns: number of CPUs present including the one just registered
 */

int platformRegisterCPU(void *cpu) {
    if(!cpus) cpus = cpu;
    last->next = cpu;
    last = cpu;
    return cpuCount++;
}

/* platformCountCPU(): returns the CPU count */

int platformCountCPU() {
    return cpuCount;
}

/* platformGetCPU(): returns a CPU structure associated with an index
 * params: n - index
 * returns: pointer to CPU structure, NULL if not present
 */

void *platformGetCPU(int n) {
    if(n >= cpuCount) return NULL;

    PlatformCPU *cpu = cpus;
    for(int i = 0; i < n; i++) {
        cpu = cpu->next;
    }

    return cpu;
}

/* smpBoot(): boots all other processors in an SMP system
 * params: none
 * returns: total number of CPUs running, including the boot CPU
 */

int smpBoot() {
    if(cpuCount < 2) return 1;

    KDEBUG("attempt to start %d application processors...\n", cpuCount - runningCpuCount);

    PlatformCPU *cpu;

    for(int i = 0; i < cpuCount; i++) {
        cpu = platformGetCPU(i);
        if(!cpu || cpu->bootCPU || cpu->running) continue;

        KDEBUG("starting CPU with local APIC ID 0x%02X\n", cpu->apicID);
    }

    return runningCpuCount;
}