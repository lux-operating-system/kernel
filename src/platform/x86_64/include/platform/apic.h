/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Platform-Specific Code for x86_64
 */

#pragma once

#include <stdint.h>
#include <kernel/acpi.h>

// ACPI MADT table
#define MADT_TYPE_LOCAL_APIC            0
#define MADT_TYPE_IOAPIC                1
#define MADT_TYPE_INTERRUPT_OVERRIDE    2
#define MADT_TYPE_BUS_NMI               3
#define MADT_TYPE_LOCAL_NMI             4
#define MADT_TYPE_LOCAL_APIC_OVERRIDE   5

#define MADT_LEGACY_PIC_PRESENT         1

#define MADT_LOCAL_APIC_ENABLED         0x01
#define MADT_LOCAL_APIC_ONLINE          0x02

// edge vs level and high vs low interrupts
#define MADT_INTERRUPT_LOW              0x02
#define MADT_INTERRUPT_LEVEL            0x08

#define MADT_CPU_BROADCAST              0xFF

// Local APIC Registers
#define LAPIC_ID                        0x020
#define LAPIC_VERSION                   0x030
#define LAPIC_TPR                       0x080   // task priority register
#define LAPIC_APR                       0x090   // arbitration priority register
#define LAPIC_PPR                       0x0A0   // processor priority register
#define LAPIC_EOI                       0x0B0
#define LAPIC_RRD                       0x0C0   // remote read register
#define LAPIC_DEST                      0x0D0
#define LAPIC_DEST_FORMAT               0x0E0
#define LAPIC_SPURIOUS_VECTOR           0x0F0
#define LAPIC_ERROR                     0x280
#define LAPIC_INT_COMMAND_LOW           0x300
#define LAPIC_INT_COMMAND_HIGH          0x310
#define LAPIC_LVT_TIMER                 0x320
#define LAPIC_LVT_LINT0                 0x350
#define LAPIC_LVT_LINT1                 0x360
#define LAPIC_LVT_ERROR                 0x370
#define LAPIC_TIMER_INITIAL             0x380
#define LAPIC_TIMER_CURRENT             0x390
#define LAPIC_TIMER_DIVIDE              0x3E0

#define LAPIC_LVT_MASK                  (1 << 16)
#define LAPIC_LVT_LEVEL                 (1 << 15)
#define LAPIC_LVT_LOW                   (1 << 13)

#define LAPIC_TIMER_ONE_SHOT            (0 << 17)
#define LAPIC_TIMER_PERIODIC            (1 << 17)
#define LAPIC_TIMER_TSC_DEADLINE        (2 << 17)
#define LAPIC_TIMER_IRQ                 0xFE        // use INT 0xFE for the timer

#define LAPIC_TIMER_DIVIDER_2           0x00
#define LAPIC_TIMER_DIVIDER_4           0x01
#define LAPIC_TIMER_DIVIDER_8           0x02
#define LAPIC_TIMER_DIVIDER_16          0x03
#define LAPIC_TIMER_DIVIDER_32          0x08
#define LAPIC_TIMER_DIVIDER_64          0x09
#define LAPIC_TIMER_DIVIDER_128         0x0A
#define LAPIC_TIMER_DIVIDER_1           0x0B

// Local APIC Interrupt Command
#define LAPIC_INT_CMD_INIT              (5 << 8)
#define LAPIC_INT_CMD_STARTUP           (6 << 8)
#define LAPIC_INT_CMD_DELIVERY          (1 << 12)   // set to ZERO on success
#define LAPIC_INT_CMD_LEVEL_DEASSERT    (0 << 14)
#define LAPIC_INT_CMD_LEVEL_ASSERT      (1 << 14)
#define LAPIC_INT_CMD_EDGE              (0 << 15)
#define LAPIC_INT_CMD_LEVEL             (1 << 15)

// Local APIC MSR
#define MSR_LAPIC                       0x1B
#define MSR_LAPIC_ENABLED               (1 << 11)

// I/O APIC Registers
#define IOAPIC_REGSEL                   0x00
#define IOAPIC_IOWIN                    0x10

#define IOAPIC_ID                       0x00
#define IOAPIC_VER                      0x01
#define IOAPIC_ARB_ID                   0x02
#define IOAPIC_REDIRECTION              0x10

// I/O APIC Redirection Register
#define IOAPIC_RED_FIXED                (0x00 << 8)
#define IOAPIC_RED_LOWEST               (0x01 << 8)
#define IOAPIC_RED_SMI                  (0x02 << 8)
#define IOAPIC_RED_NMI                  (0x04 << 8)
#define IOAPIC_RED_INIT                 (0x05 << 8)

#define IOAPIC_RED_PHYSICAL             0x00000000
#define IOAPIC_RED_LOGICAL              0x00000800
#define IOAPIC_RED_BUSY                 0x00001000
#define IOAPIC_RED_ACTIVE_LOW           0x00002000
#define IOAPIC_RED_ACTIVE_HIGH          0x00000000
#define IOAPIC_RED_LEVEL                0x00008000
#define IOAPIC_RED_EDGE                 0x00000000
#define IOAPIC_RED_MASK                 0x00010000

// Base of IRQ Routing
#define IOAPIC_INT_BASE                 0x20    // map IRQ 0 to INT 0x20, and so on

typedef struct {
    ACPIStandardHeader header;
    uint32_t localAPIC;
    uint32_t legacyPIC;
    uint8_t entries[];
} __attribute__((packed)) ACPIMADT;

typedef struct {
    uint8_t type;
    uint8_t length;

    uint8_t procID;     // ACPI ID
    uint8_t apicID;     // local APIC ID
    uint32_t flags;
} __attribute__((packed)) ACPIMADTLocalAPIC;

typedef struct {
    uint8_t type;
    uint8_t length;

    uint8_t apicID;
    uint8_t reserved;
    uint32_t mmioBase;
    uint32_t gsi;           // global system interrupt
} __attribute__((packed)) ACPIMADTIOAPIC;

typedef struct {
    uint8_t type;
    uint8_t length;

    uint8_t bus;
    uint8_t sourceIRQ;
    uint32_t gsi;
    uint16_t flags;
} __attribute__((packed)) ACPIMADTInterruptOverride;

typedef struct {
    uint8_t type;
    uint8_t length;

    uint8_t source;
    uint8_t reserved;
    uint16_t flags;
    uint32_t gsi;
} __attribute__((packed)) ACPIMADTBusNMI;

typedef struct {
    uint8_t type;
    uint8_t length;

    uint8_t procID;         // ACPI ID not APIC ID!!!
    uint16_t flags;
    uint8_t lint;
} __attribute__((packed)) ACPIMADTLocalNMI;

typedef struct {
    uint8_t type;
    uint8_t length;

    uint16_t reserved;
    uint64_t mmioBase;
} __attribute__((packed)) ACPIMADTLocalAPICOverride;

typedef struct IOAPIC {
    uint8_t version, apicID;
    uintptr_t mmio;
    uint8_t gsi, count;
    struct IOAPIC *next;
} IOAPIC;

typedef struct IRQOverride {
    uint8_t bus, source;
    uint8_t gsi;
    int level, low;
    struct IRQOverride *next;
} IRQOverride;

typedef struct BusNMI {
    uint8_t source;
    int level, low;
    uint32_t gsi;
    struct BusNMI *next;
} BusNMI;

typedef struct LocalNMI {
    uint8_t procID;
    int level, low;
    uint8_t lint;
    struct LocalNMI *next;
} LocalNMI;

int apicInit();
void lapicWrite(uint32_t, uint32_t);
uint32_t lapicRead(uint32_t);
int apicTimerInit();
uint64_t apicTimerFrequency();
void timerHandlerStub();

int ioapicRegister(IOAPIC *);
int ioapicCount();
int ioapicInit();
IOAPIC *ioapicFindIndex(int);
IOAPIC *ioapicFindIRQ(int);
void ioapicWrite(IOAPIC *, uint32_t, uint32_t);
uint32_t ioapicRead(IOAPIC *, uint32_t);
int ioapicMask(int, int);

int lnmiRegister(LocalNMI *);
int lnmiCount();
int lnmiConfigure();

int overrideIRQRegister(IRQOverride *);
int overrideIRQCount();
IRQOverride *findOverrideIRQ(uint64_t);
