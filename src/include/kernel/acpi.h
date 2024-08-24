/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

#pragma once

#include <stdint.h>
#include <kernel/boot.h>

typedef struct {
    // revision == 0
    char signature[8];      // 'RSD PTR '
    uint8_t checksum;
    char oem[6];
    uint8_t revision;
    uint32_t rsdt;

    // revision >= 2
    uint32_t length;
    uint64_t xsdt;
    uint8_t extendedChecksum;
    uint8_t reserved[3];
} __attribute__((packed)) ACPIRSDP;

typedef struct {
    char signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oem[6];
    char oemTable[8];
    uint32_t oemRevision;
    uint32_t creator;
    uint32_t creatorRevision;
} __attribute__((packed)) ACPIStandardHeader;

typedef struct {
    ACPIStandardHeader header;
    uint32_t tables[];
} __attribute__((packed)) ACPIRSDT;

typedef struct {
    ACPIStandardHeader header;
    uint64_t tables[];
} __attribute__((packed)) ACPIXSDT;

int acpiInit(KernelBootInfo *);
void *acpiFindTable(const char *, int);
