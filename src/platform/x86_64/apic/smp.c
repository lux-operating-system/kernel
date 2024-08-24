/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Platform-Specific Code for x86_64
 */

/* Symmetric Multiprocessing Implementation */

#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <platform/platform.h>
#include <platform/smp.h>
#include <platform/x86_64.h>
#include <platform/apic.h>
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

/* apMain(): entry points for application processors */

int apMain() {
    while(1);
}

/* smpBoot(): boots all other processors in an SMP system
 * params: none
 * returns: total number of CPUs running, including the boot CPU
 */

int smpBoot() {
    if(cpuCount < 2) return 1;

    KDEBUG("attempt to start %d application processors...\n", cpuCount - runningCpuCount);

    // set up the AP entry point
    apEntryVars[AP_ENTRY_GDTR] = (uint32_t)&gdtr;
    apEntryVars[AP_ENTRY_IDTR] = (uint32_t)&idtr;
    apEntryVars[AP_ENTRY_CR3] = readCR3();
    apEntryVars[AP_ENTRY_NEXT_LOW] = (uint32_t)&apMain;
    apEntryVars[AP_ENTRY_NEXT_HIGH] = (uint32_t)((uintptr_t)&apMain >> 32);

    PlatformCPU *cpu;

    for(int i = 0; i < cpuCount; i++) {
        cpu = platformGetCPU(i);
        if(!cpu || cpu->bootCPU || cpu->running) continue;

        KDEBUG("starting CPU with local APIC ID 0x%02X\n", cpu->apicID);

        // allocate a stack for the AP
        void *stack = calloc(AP_STACK_SIZE, 1);
        if(!stack) {
            KERROR("failed to allocate memory for AP stack\n");
            while(1);
        }

        uintptr_t stackPtr = (uintptr_t)(stack + AP_STACK_SIZE);
        apEntryVars[AP_ENTRY_STACK_LOW] = (uint32_t)stackPtr;
        apEntryVars[AP_ENTRY_STACK_HIGH] = (uint32_t)((uintptr_t)stackPtr >> 32);

        // copy the AP entry into low memory
        memcpy((void *)0x1000, apEntry, AP_ENTRY_SIZE);

        // send an INIT IPI
        lapicWrite(LAPIC_INT_COMMAND_HIGH, cpu->apicID << 24);
        lapicWrite(LAPIC_INT_COMMAND_LOW, LAPIC_INT_CMD_INIT | LAPIC_INT_CMD_LEVEL_NORMAL);

        // wait for delivery
        while(lapicRead(LAPIC_INT_COMMAND_LOW) & LAPIC_INT_CMD_DELIVERY);

        // deassert the INIT IPI
        lapicWrite(LAPIC_INT_COMMAND_HIGH, cpu->apicID << 24);
        lapicWrite(LAPIC_INT_COMMAND_LOW, LAPIC_INT_CMD_INIT | LAPIC_INT_CMD_LEVEL_DEASSERT);

        // and again wait for delivery
        while(lapicRead(LAPIC_INT_COMMAND_LOW) & LAPIC_INT_CMD_DELIVERY);

        // startup IPI
        lapicWrite(LAPIC_INT_COMMAND_HIGH, cpu->apicID << 24);
        lapicWrite(LAPIC_INT_COMMAND_LOW, LAPIC_INT_CMD_STARTUP | 0x01);    // the page we copied the AP entry to

        while(lapicRead(LAPIC_INT_COMMAND_LOW) & LAPIC_INT_CMD_DELIVERY);
        
        // check that the CPU actually started
        uint32_t volatile *life = (uint32_t volatile *)0x1FE0;    // the AP will set this flag to one when it boots
        while(!*life);

        cpu->running = true;
        runningCpuCount++;
    }

    return runningCpuCount;
}
