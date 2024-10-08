/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Platform-Specific Code for x86_64
 */

/* Non-Maskable Interrupt Implementation */

/* Intel Software Developer's Manual Volume 3A Part 1 Chapter 11.5.1:
 *
 * The trigger mode of local APIC NMIs is always set to be edge-sensitive, and
 * that of local APIC ExtINTs is always set to level-sensitive. LINT1 is always
 * hardwired to level-sensitive, and only LINT0 can be configured to use either
 * edge or level according to the information from the ACPI MADT.
 */

#include <platform/apic.h>
#include <platform/smp.h>
#include <kernel/logger.h>

static LocalNMI *lnmis = NULL;
static BusNMI *bnmis = NULL;
static int nlnmi = 0, nbnmi = 0;

/* lnmiRegister(): registers a local APIC NMI
 * params: lnmi - local APIC NMI structure
 * returns: count of local APIC NMIs
 */

int lnmiRegister(LocalNMI *lnmi) {
    if(!lnmis) lnmis = lnmi;
    else {
        LocalNMI *list = lnmis;
        while(list->next) list = list->next;
        list->next = lnmi;
    }

    return nlnmi++;
}

/* lnmiCount(): returns the number of local APIC NMIs
 * params: none
 * returns: number of local APIC NMIs
 */

int lnmiCount() {
    return nlnmi;
}