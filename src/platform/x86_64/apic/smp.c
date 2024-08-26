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
static int bootCPUIndex;

/* platformRegisterCPU(): registers a CPU so that the core OS code can know
 * params: cpu - pointer to a CPU structure (see smp.h for why the void type)
 * returns: number of CPUs present including the one just registered
 */

int platformRegisterCPU(void *cpu) {
    PlatformCPU *c = cpu;
    if(c->bootCPU) bootCPUIndex = cpuCount;

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

/* smpCPUInfoSetup(): constructs per-CPU kernel info structure and stores it in
 * the kernel's GS base */

void smpCPUInfoSetup() {
    // enable FS/GS segmentation
    writeCR4(readCR4() | CR4_FSGSBASE);

    CPUIDRegisters regs;
    readCPUID(1, &regs);
    uint8_t apicID = regs.ebx >> 24;

    // find the CPU with a matching local APIC ID
    PlatformCPU *cpu = cpus;
    int i = 0;
    while(cpu) {
        if(cpu->apicID == apicID) break;
        cpu = cpu->next;
        i++;
    }

    if(!cpu) {
        KERROR("could not identify CPU with local APIC 0x%02X\n", apicID);
        while(1);
    }

    KernelCPUInfo *info = calloc(1, sizeof(KernelCPUInfo));
    if(!info) {
        KERROR("could not allocate memory for per-CPU info struct for CPU %d\n", i);
        while(1);
    }

    info->cpuIndex = i;
    info->cpu = cpu;
    
    writeMSR(MSR_FS_BASE, 0);
    writeMSR(MSR_GS_BASE, 0);
    writeMSR(MSR_GS_BASE_KERNEL, (uint64_t)info);

    //KDEBUG("per-CPU kernel info struct for CPU %d is at 0x%08X\n", info->cpuIndex, (uint64_t)info);
}

/* apMain(): entry points for application processors */

int apMain() {
    // set up per-kernel CPU info
    smpCPUInfoSetup();
    enableIRQs();

    // set up the local APIC
    // on the bootstrap CPU this is done by apicTimerInit(), but we don't need
    // to rerun it to not waste time recalibrating the APIC when we already did

    // enable the local APIC (this should probably never be necessary because
    // if the APIC wasn't enabled then how did we even boot to this point?)
    uint64_t apic = readMSR(MSR_LAPIC);
    if(!(apic & MSR_LAPIC_ENABLED)) {
        writeMSR(MSR_LAPIC, apic | MSR_LAPIC_ENABLED);
    }
    lapicWrite(LAPIC_TPR, 0);       // enable all interrupts
    lapicWrite(LAPIC_DEST_FORMAT, lapicRead(LAPIC_DEST_FORMAT) | 0xF0000000);   // flat mode
    lapicWrite(LAPIC_SPURIOUS_VECTOR, 0x1FF);

    // set up per-CPU kernel info structure

    // APIC timer
    lapicWrite(LAPIC_TIMER_INITIAL, 0);     // disable timer so we can set up
    lapicWrite(LAPIC_LVT_TIMER, LAPIC_TIMER_PERIODIC | LAPIC_LVT_MASK | LAPIC_TIMER_IRQ);
    lapicWrite(LAPIC_TIMER_DIVIDE, LAPIC_TIMER_DIVIDER_1);
    lapicWrite(LAPIC_LVT_TIMER, lapicRead(LAPIC_LVT_TIMER) & ~LAPIC_LVT_MASK);
    lapicWrite(LAPIC_TIMER_INITIAL, apicTimerFrequency() / PLATFORM_TIMER_FREQUENCY);

    // we don't need to install an IRQ handler because all CPUs share the same
    // GDT/IDT; the same IRQ handler is valid for both the boot CPU and APs

    while(1);
}

/* smpBoot(): boots all other processors in an SMP system
 * params: none
 * returns: total number of CPUs running, including the boot CPU
 */

int smpBoot() {
    if(cpuCount < 2) {
        return 1;
    }

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
        // NOTE: we're using calloc() and not malloc() here to force a write to
        // the allocated memory
        // this allows the boot CPU to handle the page fault and allocate true
        // physical memory for the AP CPU, which would not be able to handle a
        // page fault yet because the page fault would require a stack while
        // literally also trying to allocate a stack, crashing the entire CPU
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

/* platformWhichCPU(): platform-independent function to identify the running CPU
 * params: none
 * returns: index of the running CPU
 */

int platformWhichCPU() {
    return getKernelCPUInfo()->cpuIndex;
}
