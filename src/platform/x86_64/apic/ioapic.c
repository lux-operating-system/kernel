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

/* ioapicInit(): initializes I/O APICs
 * params: none
 * returns: number of I/O APICs initialized
 */

int ioapicInit() {
    if(!count) {
        KERROR("no I/O APIC is present\n");
        for(;;);
    }


}