/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Platform-Specific Code for x86_64
 */

#pragma once

#include <stdint.h>
#include <kernel/acpi.h>

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

int apicInit();
