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
#include <platform/tss.h>
#include <platform/apic.h>
#include <kernel/logger.h>
#include <kernel/memory.h>

static PlatformCPU *cpus = NULL;
static PlatformCPU *last = NULL;
static size_t cpuCount = 0;
static size_t runningCpuCount = 1;      // boot CPU
static int bootCPUIndex;
static KernelCPUInfo *bootCPUInfo;
static uint32_t apBooted = 0;

extern void syscallEntry();

/* platformRegisterCPU(): registers a CPU so that the core OS code can know
 * params: cpu - pointer to a CPU structure (see smp.h for why the void type)
 * returns: number of CPUs present including the one just registered
 */

int platformRegisterCPU(void *cpu) {
    PlatformCPU *c = cpu;
    if(c->bootCPU) bootCPUIndex = cpuCount;

    if(!cpus) cpus = cpu;
    if(last) last->next = cpu;
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
 * the kernel's GS base, also sets up a per-CPU GDT and TSS and stack */

void smpCPUInfoSetup() {
    // enable FS/GS segmentation
    writeCR4(readCR4() | CR4_FSGSBASE);

    CPUIDRegisters regs;
    readCPUID(1, &regs);
    uint8_t apicID = regs.ebx >> 24;

    // enable fast fxsave/fxrstor if supported
    memset(&regs, 0, sizeof(CPUIDRegisters));
    readCPUID(0x80000001, &regs);

    if(regs.edx & (1 << 25)) {
        writeMSR(MSR_EFER, readMSR(MSR_EFER) | MSR_EFER_FFXSR);
    }

    // enable fast syscall/sysret instructions
    // CS = kernelSegmentBase; SS = kernelSegmentBase+8
    uint16_t kernelSegmentBase = (GDT_KERNEL_CODE << 3);

    // CS = userSegmentBase+16; SS = userSegmentBase+8
    // genuine question what the fuck they were smoking when they made this and
    // swapped the order between kernel and user mode?
    uint16_t userSegmentBase = (GDT_USER_DATA << 3) - 8;
    uint64_t syscallSegments = ((uint64_t)userSegmentBase << 48) | ((uint64_t)kernelSegmentBase << 32);

    writeMSR(MSR_STAR, syscallSegments);            // kernel/user segments
    writeMSR(MSR_LSTAR, (uint64_t)syscallEntry);    // 64-bit entry point
    writeMSR(MSR_CSTAR, 0);                         // non-existent 32-bit entry point
    writeMSR(MSR_SFMASK, ~0x202);

    writeMSR(MSR_EFER, readMSR(MSR_EFER) | MSR_EFER_SYSCALL);

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

    if(cpu->bootCPU) bootCPUInfo = info;

    //KDEBUG("per-CPU kernel info struct for CPU %d is at 0x%08X\n", info->cpuIndex, (uint64_t)info);

    tssSetup();
}

/* apMain(): entry points for application processors */

int apMain() {
    // reload the higher half GDT and IDT
    loadGDT(&gdtr);
    loadIDT(&idtr);

    // reset the segments
    resetSegments(GDT_KERNEL_CODE, PRIVILEGE_KERNEL);

    // reset paging
    writeCR3((uint64_t)platformGetPagingRoot());
    smpCPUInfoSetup();

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

    uint32_t volatile *complete = (uint32_t volatile *)&apBooted;
    *complete = 1;

    while(1) {
        enableIRQs();
        halt();     // wait for the scheduler to decide to do something
    }
}

/* smpBoot(): boots all other processors in an SMP system
 * params: none
 * returns: total number of CPUs running, including the boot CPU
 */

int smpBoot() {
    if(cpuCount < 2) {
        return 1;
    }

    // disable caching temporarily
    writeCR0(readCR0() | CR0_CACHE_DISABLE);

    KDEBUG("attempt to start %d application processors...\n", cpuCount - runningCpuCount);

    // set up the AP entry point
    // copy the GDTR and IDTR into low memory temporarily
    GDTR *lowGDTR = (GDTR *)vmmMMIO(0x2000, true);
    IDTR *lowIDTR = (IDTR *)vmmMMIO(0x2010, true);
    memcpy(lowGDTR, &gdtr, sizeof(GDTR));
    memcpy(lowIDTR, &idtr, sizeof(IDTR));

    // the AP will not be able to read a full 64-bit address immediately on startup, so account for that
    // we will reload the GDT and IDT after reaching 64-bit long mode
    lowGDTR->base -= KERNEL_BASE_ADDRESS;
    lowIDTR->base -= KERNEL_BASE_ADDRESS;

    // create a temporary mapping of low memory
    uintptr_t tempCR3 = pmmAllocate();
    if(!tempCR3) {
        KERROR("unable to allocate memory for temporary AP boot paging\n");
        while(1);
    }

    memcpy((void *)vmmMMIO(tempCR3, true), (const void *)vmmMMIO(readCR3() & ~(PAGE_SIZE-1), true), PAGE_SIZE);

    writeCR3(tempCR3);      // temporarily use this instead

    // identity map first 8 MB for the AP
    // this will give it access to (most of) the kernel's physical memory
    for(int i = 0; i < 2048; i++) {
        platformMapPage(i * PAGE_SIZE, i * PAGE_SIZE, PLATFORM_PAGE_PRESENT | PLATFORM_PAGE_EXEC | PLATFORM_PAGE_WRITE);
    }

    apEntryVars[AP_ENTRY_GDTR] = (uint32_t)lowGDTR - KERNEL_BASE_ADDRESS;
    apEntryVars[AP_ENTRY_IDTR] = (uint32_t)lowIDTR - KERNEL_BASE_ADDRESS;
    apEntryVars[AP_ENTRY_CR3] = tempCR3;
    apEntryVars[AP_ENTRY_NEXT_LOW] = (uint32_t)&apMain;
    apEntryVars[AP_ENTRY_NEXT_HIGH] = (uint32_t)((uintptr_t)&apMain >> 32);

    PlatformCPU *cpu;
    uint32_t volatile *complete = (uint32_t volatile *)&apBooted;

    for(int i = 0; i < cpuCount; i++) {
        cpu = platformGetCPU(i);
        if(!cpu || cpu->bootCPU || cpu->running) continue;

        KDEBUG("starting CPU with local APIC ID 0x%02X\n", cpu->apicID);

        *complete = 0;

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
        memcpy((void *)vmmMMIO(0x1000, true), apEntry, AP_ENTRY_SIZE);

        // send an INIT IPI
        lapicWrite(LAPIC_INT_COMMAND_HIGH, cpu->apicID << 24);
        lapicWrite(LAPIC_INT_COMMAND_LOW, LAPIC_INT_CMD_INIT | LAPIC_INT_CMD_LEVEL | LAPIC_INT_CMD_LEVEL_ASSERT);

        // wait for delivery
        while(lapicRead(LAPIC_INT_COMMAND_LOW) & LAPIC_INT_CMD_DELIVERY);

        // deassert the INIT IPI
        lapicWrite(LAPIC_INT_COMMAND_HIGH, cpu->apicID << 24);
        lapicWrite(LAPIC_INT_COMMAND_LOW, LAPIC_INT_CMD_INIT | LAPIC_INT_CMD_LEVEL_DEASSERT);

        // and again wait for delivery
        while(lapicRead(LAPIC_INT_COMMAND_LOW) & LAPIC_INT_CMD_DELIVERY);

        // startup IPI
        lapicWrite(LAPIC_INT_COMMAND_HIGH, cpu->apicID << 24);
        lapicWrite(LAPIC_INT_COMMAND_LOW, LAPIC_INT_CMD_STARTUP | LAPIC_INT_CMD_LEVEL | 0x01);    // the page we copied the AP entry to

        while(lapicRead(LAPIC_INT_COMMAND_LOW) & LAPIC_INT_CMD_DELIVERY);
        
        // check that the CPU actually started
        uint32_t volatile *life = (uint32_t volatile *)0x1FE0;    // the AP will set this flag to one when it boots
        while(!*life || !*complete);

        cpu->running = true;
        runningCpuCount++;
    }

    writeCR0(readCR0() & ~(CR0_CACHE_DISABLE | CR0_NOT_WRITE_THROUGH));
    writeCR3((uint64_t)platformGetPagingRoot());
    return runningCpuCount;
}

/* platformWhichCPU(): platform-independent function to identify the running CPU
 * params: none
 * returns: index of the running CPU
 */

int platformWhichCPU() {
    return getKernelCPUInfo()->cpuIndex;
}

/* platformUptime(): global uptime, really the timer ticks for the boot CPU
 * params: none
 * returns: timer ticks elapsed on boot CPU
 */

uint64_t platformUptime() {
    if(!bootCPUInfo) return 0;
    return bootCPUInfo->uptime;
}