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

static IOAPIC *ioapics = NULL;
static int ioapicCount = 0;

/* registerIOAPIC(): registers an I/O APIC device
 * params: dev - pointer to the device structure
 * returns: total count of I/O APICs
 */

int registerIOAPIC(IOAPIC *dev) {
    if(!ioapics) ioapics = dev;
    else {
        IOAPIC *ioapic = ioapics;
        while(ioapic->next) ioapic = ioapic->next;
        ioapic->next = dev;
    }

    return ioapicCount++;
}

/* countIOAPIC(): returns the number of I/O APICs present
 * params: none
 * returns: number of I/O APICs
 */

int countIOAPIC() {
    return ioapicCount;
}

