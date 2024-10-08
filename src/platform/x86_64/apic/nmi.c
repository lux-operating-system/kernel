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
 * hardwired to edge-sensitive, and only LINT0 can be configured to use either
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

/* lnmiConfigure(): configures local APIC NMIs on a given core
 * params: none
 * returns: number of NMIs configured on this core
 */

int lnmiConfigure() {
    if(!nlnmi || !lnmis) return 0;
    int count = 0;

    PlatformCPU *cpu = getKernelCPUInfo()->cpu;
    uint8_t acpiID = cpu->procID;

    // find local APIC NMIs with a matching ACPI ID
    LocalNMI *list = lnmis;
    while(list) {
        if((list->procID == acpiID) || (list->procID == 0xFF)) {
            uint32_t config = LAPIC_LVT_NMI;
            if(list->low) config |= LAPIC_LVT_LOW;

            if(list->lint & 1) lapicWrite(LAPIC_LVT_LINT1, config);
            else lapicWrite(LAPIC_LVT_LINT0, config);
            count++;
        }

        list = list->next;
    }

    return count;
}