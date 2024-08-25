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
#include <platform/platform.h>
#include <kernel/logger.h>

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
    lapicWrite(LAPIC_TPR, 0);
    lapicWrite(LAPIC_DEST_FORMAT, 0);
    lapicWrite(LAPIC_SPURIOUS_VECTOR, 0x1FF);
    
    /* TODO: use the HPET instead of the PIT to measure timer frequency */
    // set up the PIT to wait for 1/20 of a second
    uint16_t pitFrequency = 1193180 / 100;      // PIT frequency divided by 100 Hz
    KDEBUG("using PIT to calibrate local APIC timer: starting counter 0x%04X\n", pitFrequency);

    // set up the APIC timer in one-shot mode with no interrupts
    lapicWrite(LAPIC_TIMER_INITIAL, 0);     // disable timer so we can set it up
    lapicWrite(LAPIC_LVT_TIMER, LAPIC_TIMER_ONE_SHOT | LAPIC_LVT_MASK);
    lapicWrite(LAPIC_TIMER_DIVIDE, LAPIC_TIMER_DIVIDER_1);
    lapicWrite(LAPIC_TIMER_INITIAL, 0x8FFFFFFF);    // enable timer

    uint32_t apicInitial = lapicRead(LAPIC_TIMER_CURRENT);

    outb(0x43, 0x30);   // channel 0, high and low in one transfer, mode 0
    outb(0x40, pitFrequency & 0xFF);
    outb(0x40, pitFrequency >> 8);

    uint16_t currentCounter = pitFrequency;
    uint16_t oldCurrentCounter = pitFrequency;

    while(currentCounter <= oldCurrentCounter) {
        oldCurrentCounter = currentCounter;

        outb(0x43, 0x00);       // channel 0, latch command
        currentCounter = inb(0x40);
        currentCounter |= inb(0x40) << 8;
    }

    uint32_t apicFinal = lapicRead(LAPIC_TIMER_CURRENT);

    // disable the APIC timer
    lapicWrite(LAPIC_TIMER_INITIAL, 0);
    uint64_t apicTicks = (uint64_t)apicInitial - (uint64_t)apicFinal;
    apicFrequency = apicTicks * 100;    // the PIT was set up to 100 Hz

    KDEBUG("local APIC frequency is %d MHz\n", apicFrequency / 1000 / 1000);

    // set up the local APIC timer in periodic mode and allocate interrupt 0xFE for it
    lapicWrite(LAPIC_LVT_TIMER, LAPIC_TIMER_PERIODIC | LAPIC_LVT_MASK | LAPIC_TIMER_IRQ);
    lapicWrite(LAPIC_TIMER_DIVIDE, LAPIC_TIMER_DIVIDER_1);
    installInterrupt((uint64_t)timerHandlerStub, GDT_KERNEL_CODE, PRIVILEGE_KERNEL, INTERRUPT_TYPE_INT, LAPIC_TIMER_IRQ);

    // unmask the IRQ and enable the timer
    lapicWrite(LAPIC_LVT_TIMER, lapicRead(LAPIC_LVT_TIMER) & ~LAPIC_LVT_MASK);
    lapicWrite(LAPIC_TIMER_INITIAL, apicFrequency / PLATFORM_TIMER_FREQUENCY);
}

/* apicTimerFrequency(): returns the local APIC's frequency */

uint64_t apicTimerFrequency() {
    return apicFrequency;
}

/* timerIRQ(): timer IRQ handler 
 * this is called PLATFORM_TIMER_FREQUENCY times per second */
uint64_t timerTicks = 0;
void timerIRQ() {
    timerTicks++;
    if(!(timerTicks % PLATFORM_TIMER_FREQUENCY)) {
        KDEBUG("timer irq: one second has passed\n");
    }
    platformAcknowledgeIRQ(NULL);
}
