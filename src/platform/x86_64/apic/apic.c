/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Platform-Specific Code for x86_64
 */

#include <stddef.h>
#include <stdlib.h>
#include <platform/platform.h>
#include <platform/apic.h>
#include <platform/x86_64.h>
#include <platform/smp.h>
#include <kernel/acpi.h>
#include <kernel/logger.h>

static uint64_t localAPICBase;

/* apicInit(): detects and initializes APICs */
/* this is the point where multiprocessing and interrupts are initialized,
 * setting the stage for the scheduler and the platform-independent code */

int apicInit() {
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
    CPUIDRegisters regs;
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
                PlatformCPU *cpu = malloc(sizeof(PlatformCPU));
                if(!cpu) {
                    KERROR("could not allocate memory to register CPU\n");
                    break;
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
            break;
        case MADT_TYPE_INTERRUPT_OVERRIDE:
            ACPIMADTInterruptOverride *override = (ACPIMADTInterruptOverride *)ptr;
            KDEBUG("map IRQ %d from bus 0x%02X -> GSI %d with flags 0x%04X (%s, %s)\n",
                override->sourceIRQ, override->bus, override->gsi, override->flags,
                override->flags & MADT_INTERRUPT_LEVEL ? "level" : "edge",
                override->flags & MADT_INTERRUPT_LOW ? "low" : "high");
            break;
        default:
            KWARN("unimplemented MADT entry type 0x%02X with length %d, skipping...\n", ptr[0], ptr[1]);
        }

        n += ptr[1];
        ptr += ptr[1];
    }

    /* now boot SMPs */
    smpBoot();
    while(1);
}

/* Local APIC Read/Write */

void lapicWrite(uint32_t reg, uint32_t val) {
    uint32_t volatile *ptr = (uint32_t volatile *)((uintptr_t)localAPICBase + reg);
    *ptr = val;
}

uint32_t lapicRead(uint32_t reg) {
    uint32_t volatile *ptr = (uint32_t volatile*)((uintptr_t)localAPICBase + reg);
    return *ptr;
}
