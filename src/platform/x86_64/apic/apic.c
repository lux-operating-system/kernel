/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Platform-Specific Code for x86_64
 */

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <platform/platform.h>
#include <platform/apic.h>
#include <platform/x86_64.h>
#include <platform/smp.h>
#include <kernel/acpi.h>
#include <kernel/logger.h>
#include <kernel/memory.h>

static uint64_t localAPICBase;

/* apicInit(): detects and initializes APICs */
/* this is the point where multiprocessing and interrupts are initialized,
 * setting the stage for the scheduler and the platform-independent code */

int apicInit() {
    // check for FS/GS base
    CPUIDRegisters regs;
    memset(&regs, 0, sizeof(CPUIDRegisters));
    readCPUID(7, &regs);

    if(!(regs.ebx & 1)) {
        // this is a required feature for the kernel to know which CPU it's running on
        KERROR("CPU doesn't support 64-bit FS/GS segmentation\n");
        while(1);
    }

    // check for syscall/sysret instructions
    memset(&regs, 0, sizeof(CPUIDRegisters));
    readCPUID(0x80000001, &regs);
    if(!(regs.edx & (1 << 11))) {
        KERROR("CPU doesn't support fast syscall/sysret\n");
        while(1);
    }

    ACPIMADT *madt = acpiFindTable("APIC", 0);
    if(!madt) {
        KERROR("ACPI MADT table is not present\n");
        while(1);
    }

    KDEBUG("reading ACPI MADT table...\n");

    KDEBUG("32-bit local APIC address: 0x%08X\n", madt->localAPIC);
    localAPICBase = madt->localAPIC;
    KDEBUG("legacy PIC is %s\n", madt->legacyPIC & MADT_LEGACY_PIC_PRESENT ? "present" : "absent");

    // if the legacy PIC is present, we need to disable it so it doesn't
    // interfere with the APIC
    if(madt->legacyPIC & MADT_LEGACY_PIC_PRESENT) {
        outb(0x21, 0xFF);
        outb(0xA1, 0xFF);
    }

    uint8_t *ptr = madt->entries;
    size_t n = (size_t)(ptr - (uint8_t *)madt);

    // identify the BSP
    readCPUID(1, &regs);
    uint8_t bspID = regs.ebx >> 24;

    KDEBUG("BSP local APIC ID is 0x%02X\n", bspID);

    while(n < madt->header.length) {
        switch(*ptr) {
        case MADT_TYPE_LOCAL_APIC:
            ACPIMADTLocalAPIC *localAPIC = (ACPIMADTLocalAPIC *)ptr;
            KDEBUG("local APIC with ACPI ID 0x%02X APIC ID 0x%02X flags 0x%08X (%s)\n", localAPIC->procID,
                localAPIC->apicID, localAPIC->flags,
                localAPIC->flags & MADT_LOCAL_APIC_ENABLED ? "enabled" : "disabled");

            // register the CPU with the platform-independent code if it's enabled
            if(localAPIC->flags & MADT_LOCAL_APIC_ENABLED) {
                PlatformCPU *cpu = calloc(1, sizeof(PlatformCPU));
                if(!cpu) {
                    KERROR("could not allocate memory to register CPU\n");
                    while(1);
                }

                cpu->apicID = localAPIC->apicID;
                cpu->procID = localAPIC->procID;
                cpu->bootCPU = (localAPIC->apicID == bspID);
                cpu->running = cpu->bootCPU;
                cpu->next = NULL;

                platformRegisterCPU(cpu);
            }
            break;
        case MADT_TYPE_IOAPIC:
            ACPIMADTIOAPIC *ioapic = (ACPIMADTIOAPIC *)ptr;
            KDEBUG("I/O APIC with APIC ID 0x%02X GSI base %d MMIO base 0x%08X\n", ioapic->apicID,
                ioapic->gsi, ioapic->mmioBase);

            // register the I/O APIC
            IOAPIC *dev = calloc(1, sizeof(IOAPIC));
            if(!dev) {
                KERROR("could not allocate memory to register I/O APIC\n");
                while(1);
            }

            dev->apicID = ioapic->apicID;
            dev->gsi = ioapic->gsi;
            dev->mmio = ioapic->mmioBase;
            ioapicRegister(dev);
            break;
        case MADT_TYPE_INTERRUPT_OVERRIDE:
            ACPIMADTInterruptOverride *override = (ACPIMADTInterruptOverride *)ptr;
            KDEBUG("map IRQ %d from bus 0x%02X -> GSI %d with flags 0x%04X (%s, %s)\n",
                override->sourceIRQ, override->bus, override->gsi, override->flags,
                override->flags & MADT_INTERRUPT_LEVEL ? "level" : "edge",
                override->flags & MADT_INTERRUPT_LOW ? "low" : "high");
            
            IRQOverride *irqor = calloc(1, sizeof(IRQOverride));
            if(!irqor) {
                KERROR("could not allocate memory for IRQ override\n");
                while(1);
            }

            irqor->bus = override->bus;
            irqor->gsi = override->gsi;
            irqor->source = override->sourceIRQ;
            if(override->flags & MADT_INTERRUPT_LEVEL) irqor->level = 1;
            if(override->flags & MADT_INTERRUPT_LOW) irqor->low = 1;

            overrideIRQRegister(irqor);
            break;
        case MADT_TYPE_LOCAL_NMI:
            ACPIMADTLocalNMI *lnmiEntry = (ACPIMADTLocalNMI *) ptr;
            KDEBUG("local APIC NMI on ACPI ID 0x%02X LINT#%d with flags 0x%04X (%s, %s)\n",
                lnmiEntry->procID, lnmiEntry->lint & 1, lnmiEntry->flags,
                lnmiEntry->flags & MADT_INTERRUPT_LEVEL ? "level" : "edge",
                lnmiEntry->flags & MADT_INTERRUPT_LOW ? "low" : "high");
            
            LocalNMI *lnmi = calloc(1, sizeof(LocalNMI));
            if(!lnmi) {
                KERROR("could not allocate memory for local APIC NMI\n");
                while(1);
            }

            lnmi->procID = lnmiEntry->procID;
            lnmi->lint = lnmiEntry->lint;
            if(lnmiEntry->flags & MADT_INTERRUPT_LEVEL) lnmi->level = 1;
            if(lnmiEntry->flags & MADT_INTERRUPT_LOW) lnmi->low = 1;

            lnmiRegister(lnmi);
            break;
        default:
            KWARN("unimplemented MADT entry type 0x%02X with length %d, skipping...\n", ptr[0], ptr[1]);
        }

        n += ptr[1];
        ptr += ptr[1];
    }

    /* continue booting with info acquired from ACPI */
    smpCPUInfoSetup();      // info structure for the boot CPU
    apicTimerInit();        // local APIC timer
    smpBoot();              // start up other non-boot CPUs
    ioapicInit();           // I/O APICs

    return 0;
}

/* Local APIC Read/Write */

void lapicWrite(uint32_t reg, uint32_t val) {
    uint32_t volatile *ptr = (uint32_t volatile *)((uintptr_t)vmmMMIO(localAPICBase + reg, true));
    *ptr = val;
}

uint32_t lapicRead(uint32_t reg) {
    uint32_t volatile *ptr = (uint32_t volatile*)((uintptr_t)vmmMMIO(localAPICBase + reg, true));
    return *ptr;
}

/* platformAcknowledgeIRQ(): platform-independent function that will be called
 * at the end of an IRQ handler
 * params: unused - pointer to data that may be necessary for IRQ acknowledgement; unused on x86_64
 * returns: nothing
 */

void platformAcknowledgeIRQ(void *unused) {
    lapicWrite(LAPIC_EOI, 0);
}
