/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Platform-Specific Code for x86_64
 */

/* I/O APIC Driver */

#define MAX_IRQS            47      // TODO: bump this up

#include <errno.h>
#include <stddef.h>
#include <platform/apic.h>
#include <platform/x86_64.h>
#include <platform/smp.h>
#include <platform/platform.h>
#include <kernel/logger.h>
#include <kernel/memory.h>
#include <kernel/irq.h>

static IOAPIC *ioapics = NULL;
static int count = 0;
static int max = 0;

extern uint64_t dispatchIRQTable[];

/* ioapicRegister(): registers an I/O APIC device
 * params: dev - pointer to the device structure
 * returns: total count of I/O APICs
 */

int ioapicRegister(IOAPIC *dev) {
    if(!ioapics) ioapics = dev;
    else {
        IOAPIC *ioapic = ioapics;
        while(ioapic->next) ioapic = ioapic->next;
        ioapic->next = dev;
    }

    return count++;
}

/* ioapicCount(): returns the number of I/O APICs present
 * params: none
 * returns: number of I/O APICs
 */

int ioapicCount() {
    return count;
}

/* ioapicFindIndex(): finds an I/O APIC by index
 * params: index - index to find
 * returns: pointer to I/O APIC structure, NULL if non-existent
 */

IOAPIC *ioapicFindIndex(int index) {
    if(index >= count) return NULL;

    int i = 0;
    IOAPIC *ioapic = ioapics;
    while(ioapic) {
        if(i == index) return ioapic;
        
        ioapic = ioapic->next;
        i++;
    }

    return NULL;
}

/* ioapicFindIRQ(): finds an I/O APIC by IRQ
 * params: irq - IRQ to find
 * returns: pointer to I/O APIC structure, NULL if non-existent
 */

IOAPIC *ioapicFindIRQ(int irq) {
    IOAPIC *ioapic = ioapics;
    while(ioapic) {
        if((irq >= ioapic->gsi) && irq < (ioapic->gsi + ioapic->count))
            return ioapic;
        else ioapic = ioapic->next;
    }

    return NULL;
}

/* ioapicWrite(): writes to an I/O APIC register
 * params: ioapic - the I/O APIC device to write to
 * params: index - register index
 * params: value - value to write
 * returns: nothing
 */

void ioapicWrite(IOAPIC *ioapic, uint32_t index, uint32_t value) {
    uint32_t volatile *selector = (uint32_t volatile *)((uintptr_t)vmmMMIO(ioapic->mmio + IOAPIC_REGSEL, true));
    uint32_t volatile *window = (uint32_t volatile *)((uintptr_t)vmmMMIO(ioapic->mmio + IOAPIC_IOWIN, true));

    *selector = index;
    *window = value;
}

/* ioapicRead(): reads from an I/O APIC register
 * params: ioapic - the I/O APIC device to read from
 * params: index - register index
 * returns: value from register
 */

uint32_t ioapicRead(IOAPIC *ioapic, uint32_t index) {
    uint32_t volatile *selector = (uint32_t volatile *)((uintptr_t)vmmMMIO(ioapic->mmio + IOAPIC_REGSEL, true));
    uint32_t volatile *window = (uint32_t volatile *)((uintptr_t)vmmMMIO(ioapic->mmio + IOAPIC_IOWIN, true));

    *selector = index;
    return *window;
}

/* ioapicMask(): configures the mask of an IRQ
 * params: irq - IRQ to configure
 * params: mask - 0 to unmask, 1 to mask
 * returns: zero on success
 */

int ioapicMask(int irq, int mask) {
    IOAPIC *ioapic = ioapicFindIRQ(irq);
    if(!ioapic) return -1;

    uint32_t index = IOAPIC_REDIRECTION + (irq * 2);
    if(mask)
        ioapicWrite(ioapic, index, ioapicRead(ioapic, index) | IOAPIC_RED_MASK);
    else
        ioapicWrite(ioapic, index, ioapicRead(ioapic, index) & ~IOAPIC_RED_MASK);

    return 0;
}

/* ioapicInit(): initializes I/O APICs
 * params: none
 * returns: number of I/O APICs initialized
 */

int ioapicInit() {
    if(!count) {
        KERROR("no I/O APIC is present\n");
        for(;;);
    }

    for(int i = 0; i < count; i++) {
        IOAPIC *ioapic = ioapicFindIndex(i);
        if(!ioapic) {
            KERROR("I/O APIC index %d is not present, memory corruption?\n", i);
            for(;;);
        }

        // read the version and max IRQ
        uint32_t val = ioapicRead(ioapic, IOAPIC_VER);
        ioapic->version = val & 0xFF;
        ioapic->count = ((val >> 16) & 0xFF) + 1;

        if((ioapic->gsi + ioapic->count - 1) > max) 
            max = ioapic->gsi + ioapic->count - 1;

        // and mask all IRQs
        for(int j = ioapic->gsi; j < (ioapic->gsi + ioapic->count); j++)
            ioapicMask(j, 1);

        KDEBUG("I/O APIC version 0x%02X @ 0x%X routing IRQs %d-%d\n", ioapic->version, ioapic->mmio, ioapic->gsi, ioapic->gsi+ioapic->count-1);
    }

    KDEBUG("%d I/O APIC%s can route a total of %d IRQs\n", count, count != 1 ? "s" : "", max+1);

    if(max > MAX_IRQS) {
        KWARN("kernel is currently limited to %d IRQs, only figuring the first %d\n", MAX_IRQS+1, MAX_IRQS+1);
        max = MAX_IRQS;
    }

    // install IRQ handler stubs
    for(int i = 0; i < max; i++)
        installInterrupt(dispatchIRQTable[i], GDT_KERNEL_CODE, PRIVILEGE_KERNEL, INTERRUPT_TYPE_INT, i + IOAPIC_INT_BASE);

    return count;
}

/* platformGetMaxIRQ(): returns the maximum IRQ on the system
 * params: none
 * returns: maximum IRQ pin
 */

int platformGetMaxIRQ() {
    return max;
}

/* platformConfigureIRQ(): configures and unmasks an IRQ on the I/O APIC
 * params: t - calling thread
 * params: pin - IRQ number, platform-dependent
 * params: h - IRQ handler structure
 * returns: interrupt pin on success, negative error code on fail
 */

int platformConfigureIRQ(Thread *t, int pin, IRQHandler *h) {
    // check for IRQ overrides
    uint32_t low = 0;
    IRQOverride *override = findOverrideIRQ(pin);
    if(override) {
        if(override->level) low |= IOAPIC_RED_LEVEL;
        if(override->low) low |= IOAPIC_RED_ACTIVE_LOW;
        pin = override->gsi;
    } else {
        if(h->level) low |= IOAPIC_RED_LEVEL;
        if(!h->high) low |= IOAPIC_RED_ACTIVE_LOW;
    }

    // find which I/O APIC implements this IRQ line
    IOAPIC *ioapic = ioapicFindIRQ(pin);
    if(!ioapic) return -EIO;

    int line = pin - ioapic->gsi;   // line on this particular I/O APIC

    // map the IRQ to a CPU interrupt
    low |= (pin + IOAPIC_INT_BASE);

    // cycle through CPUs that handle IRQs
    int cpuIndex = pin % platformCountCPU();
    PlatformCPU *cpu = platformGetCPU(cpuIndex);
    if(!cpu) cpu = platformGetCPU(0);       // boot CPU
    
    cpuIndex = cpu->apicID & 0x0F;     // I/O APIC can only target 16 cpus in physical mode

    uint32_t high = cpuIndex << 24;

    // write the high dword first because the low will unmask
    ioapicWrite(ioapic, IOAPIC_REDIRECTION + (line*2) + 1, high);
    ioapicWrite(ioapic, IOAPIC_REDIRECTION + (line*2), low);

    return pin;
}