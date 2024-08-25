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

#include <platform/apic.h>
#include <platform/x86_64.h>
#include <kernel/logger.h>

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
    lapicWrite(LAPIC_SPURIOUS_VECTOR, 0x1FF);
    uint64_t apic = readMSR(MSR_LAPIC);
    if(!(apic & MSR_LAPIC_ENABLED)) {
        writeMSR(MSR_LAPIC, apic | MSR_LAPIC_ENABLED);
    }
    
    /* TODO: use the HPET instead of the PIT to measure timer frequency */
    // set up the PIT to wait for 1/20 of a second
    uint16_t pitFrequency = 1193180 / 20;      // PIT frequency divided by 20 Hz
    KDEBUG("using PIT to calibrate local APIC timer: starting counter 0x%04X\n", pitFrequency);

    // set up the APIC timer in one-shot mode with no interrupts
    lapicWrite(LAPIC_LVT_TIMER, LAPIC_TIMER_ONE_SHOT | LAPIC_LVT_MASK);
    lapicWrite(LAPIC_TIMER_DIVIDE, LAPIC_TIMER_DIVIDER_1);
    lapicWrite(LAPIC_TIMER_INITIAL, 0xFFFFFFFF);

    uint32_t apicInitial = lapicRead(LAPIC_TIMER_CURRENT);

    outb(0x43, 0x30);   // channel 0, high and low in one transfer, mode 0
    outb(0x41, pitFrequency & 0xFF);
    outb(0x41, pitFrequency >> 8);

    uint16_t currentCounter = pitFrequency;
    uint16_t oldCurrentCounter = pitFrequency;

    while(currentCounter <= oldCurrentCounter) {
        oldCurrentCounter = currentCounter;

        outb(0x43, 0x30);
        currentCounter = inb(0x41);
        currentCounter |= inb(0x41) << 8;
    }

    uint32_t apicFinal = lapicRead(LAPIC_TIMER_CURRENT);
    uint32_t apicTicks = apicFinal - apicInitial;
    uint64_t apicFrequency = apicTicks * 20;    // the PIT was set up to 20 Hz

    KDEBUG("local APIC frequency is %d MHz\n", apicFrequency / 1000 / 1000);
    while(1);
}