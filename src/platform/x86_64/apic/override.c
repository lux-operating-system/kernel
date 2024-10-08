/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Platform-Specific Code for x86_64
 */

/* IRQ Overrides for the I/O APIC */

#include <stddef.h>
#include <platform/apic.h>

static IRQOverride *overrides = NULL;
static int count = 0;

/* overrideIRQRegister(): registers an IRQ override
 * params: override - pointer to the IRQ override structure
 * returns: number of IRQ overrides present
 */

int overrideIRQRegister(IRQOverride *override) {
    if(!overrides) overrides = override;
    else {
        IRQOverride *list = overrides;
        while(list->next) list = list->next;
        list->next = override;
    }
    return count++;
}

/* overrideIRQCount(): returns the number of IRQ overrides present
 * params: none
 * returns: number of IRQ overrides
 */

int overrideIRQCount() {
    return count;
}

/* findOverrideIRQ(): returns an IRQ override for an ISA IRQ
 * params: pin - ISA IRQ number
 * returns: pointer to the IRQ override structure, NULL if not overridden
 */

IRQOverride *findOverrideIRQ(uint64_t pin) {
    if(!count) return NULL;

    IRQOverride *override = overrides;
    while(override) {
        if(override->gsi == pin) return override;
        else override = override->next;
    }

    return NULL;
}