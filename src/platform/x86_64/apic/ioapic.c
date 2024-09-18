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
static int count = 0;

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

/* ioapicConut(): returns the number of I/O APICs present
 * params: none
 * returns: number of I/O APICs
 */

int ioapicConut() {
    return count;
}

