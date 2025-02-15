/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Platform-Specific Code for x86_64
 */

/* Implementation of the Local APIC Timer */
/* This will be the main timing source on x86_64 because it is on the same
 * physical circuit as the CPU, reducing latency compared to external timers
 * like the HPET or the legacy PIT (which I don't intend to support) */

#include <stddef.h>
#include <platform/apic.h>
#include <platform/x86_64.h>
#include <platform/smp.h>
#include <platform/platform.h>
#include <kernel/logger.h>
#include <kernel/sched.h>

static uint64_t apicFrequency;

/* apicTimerInit(): initializes the local APIC timer
 * this may depend on the CMOS to calibrate the timer */

int apicTimerInit() {
    // first check if we can calculate the APIC timer's frequency
    CPUIDRegisters regs;
    readCPUID(0, &regs);
    // TODO: use this to determine CPU core and bus frequency
    /*if(regs.eax >= 0x16) {  // max function
        readCPUID(0x16, &regs);
    }*/

    // enable the local APIC
    uint64_t apic = readMSR(MSR_LAPIC);
    if(!(apic & MSR_LAPIC_ENABLED)) {
        writeMSR(MSR_LAPIC, apic | MSR_LAPIC_ENABLED);
    }
    
    lapicWrite(LAPIC_TPR, 0);       // enable all interrupts
    lapicWrite(LAPIC_DEST_FORMAT, lapicRead(LAPIC_DEST_FORMAT) | 0xF0000000);   // flat mode
    lapicWrite(LAPIC_SPURIOUS_VECTOR, 0x1FF);
    
    /* TODO: use the HPET instead of the PIT to measure timer frequency */
    // set up the PIT to wait for a fraction of a second
    uint16_t pitFrequency = 1193180 / 100;      // PIT frequency divided by 100 Hz

    // set up the APIC timer in one-shot mode with no interrupts
    lapicWrite(LAPIC_TIMER_INITIAL, 0);     // disable timer so we can set it up
    lapicWrite(LAPIC_LVT_TIMER, LAPIC_TIMER_ONE_SHOT | LAPIC_LVT_MASK);
    lapicWrite(LAPIC_TIMER_DIVIDE, LAPIC_TIMER_DIVIDER_1);
    lapicWrite(LAPIC_TIMER_INITIAL, 0xFFFFFFFF);    // enable timer

    uint32_t apicInitial = lapicRead(LAPIC_TIMER_CURRENT);

    outb(0x43, 0x30);   // channel 0, high and low in one transfer, mode 0
    outb(0x40, pitFrequency & 0xFF);
    outb(0x40, pitFrequency >> 8);

    uint16_t currentCounter = pitFrequency;
    uint16_t oldCurrentCounter = pitFrequency;

    // this way we correctly handle a zero counter, as well as an underflow
    while((currentCounter <= oldCurrentCounter) && currentCounter) {
        oldCurrentCounter = currentCounter;

        outb(0x43, 0x00);       // channel 0, latch command
        currentCounter = (uint16_t) inb(0x40);
        currentCounter |= (uint16_t) inb(0x40) << 8;
    }

    uint32_t apicFinal = lapicRead(LAPIC_TIMER_CURRENT);

    // disable the APIC timer
    lapicWrite(LAPIC_TIMER_INITIAL, 0);
    uint64_t apicTicks = (uint64_t)apicInitial - (uint64_t)apicFinal;
    apicFrequency = apicTicks * 100;    // the PIT was set up to 100 Hz

    KDEBUG("local APIC frequency is %d MHz\n", apicFrequency / 1000 / 1000);

    // ensure the hardware can go at least twice as fast as the software
    if(apicFrequency < (PLATFORM_TIMER_FREQUENCY*2)) {
        KERROR("local APIC frequency is not high enough to use as main timing source\n");
        for(;;) platformHalt();
    }

    //KDEBUG("setting up system timer at %d kHz\n", PLATFORM_TIMER_FREQUENCY / 1000);

    // set up the local APIC timer in periodic mode and allocate interrupt 0xFE for it
    lapicWrite(LAPIC_LVT_TIMER, LAPIC_TIMER_PERIODIC | LAPIC_LVT_MASK | LAPIC_TIMER_IRQ);
    lapicWrite(LAPIC_TIMER_DIVIDE, LAPIC_TIMER_DIVIDER_1);
    installInterrupt((uint64_t)timerHandlerStub, GDT_KERNEL_CODE, PRIVILEGE_KERNEL, INTERRUPT_TYPE_INT, LAPIC_TIMER_IRQ);

    // unmask the IRQ and enable the timer
    lapicWrite(LAPIC_LVT_TIMER, lapicRead(LAPIC_LVT_TIMER) & ~LAPIC_LVT_MASK);
    lapicWrite(LAPIC_TIMER_INITIAL, apicFrequency / PLATFORM_TIMER_FREQUENCY);

    return 0;
}

/* apicTimerFrequency(): returns the local APIC's frequency */

uint64_t apicTimerFrequency() {
    return apicFrequency;
}

/* timerIRQ(): timer IRQ handler 
 * this is called PLATFORM_TIMER_FREQUENCY times per second */

void timerIRQ(void *stack) {
    setLocalSched(false);
    KernelCPUInfo *info = getKernelCPUInfo();
    info->uptime++;

    // is it time for a context switch?
    if(!schedTimer()) {
        if(info->thread && info->thread->context) {
            platformSaveContext(info->thread->context, stack);
        }

        platformAcknowledgeIRQ(NULL);

        // now switch the context
        schedule();
    } else {
        platformAcknowledgeIRQ(NULL);
    }
}
