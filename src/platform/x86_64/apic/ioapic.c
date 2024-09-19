/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Platform-Specific Code for x86_64
 */

/* I/O APIC Driver */

#include <stddef.h>
#include <platform/apic.h>
#include <kernel/logger.h>
#include <kernel/memory.h>

static IOAPIC *ioapics = NULL;
static int count = 0;
static int max = 0;

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
        else ioapic = ioapic->next;
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
            KERROR("I/O APIC index %d is not present, memory corruption?\n");
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
    return count;
}

/* platformGetMaxIRQ(): returns the maximum IRQ on the system
 * params: none
 * returns: maximum IRQ pin
 */

int platformGetMaxIRQ() {
    return max;
}
