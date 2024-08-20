/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

#pragma once

#include <stdint.h>

typedef struct {
    uint8_t flags;
    uint8_t chsStart[3];
    uint8_t id;
    uint8_t chsEnd[3];
    uint32_t start;
    uint32_t size;
} __attribute__((packed)) MBRPartition;

typedef struct {
    uint32_t magic;         // 0x5346584C
    uint32_t version;       // 1

    uint8_t flags;

    /* BIOS-specific info */
    uint8_t biosBootDisk;
    uint8_t biosBootPartitionIndex;
    MBRPartition biosBootPartition;

    /* TODO: UEFI info */
    uint8_t uefiReserved[32];

    /* generic info */
    uint64_t kernelHighestAddress;
    uint64_t kernelTotalSize;
    
    uint64_t acpiRSDP;
    uint64_t highestPhysicalAddress;
    uint64_t memoryMap;
    uint8_t memoryMapSize;

    uint16_t width;
    uint16_t height;
    uint8_t bpp;
    uint64_t framebuffer;
    uint8_t redPosition;
    uint8_t redMask;
    uint8_t greenPosition;
    uint8_t greenMask;
    uint8_t bluePosition;
    uint8_t blueMask;

    uint64_t ramdisk;           // pointer
    uint64_t ramdiskSize;

    uint8_t moduleCount;
    uint64_t modules;           // pointer to pointers
    uint64_t moduleSizes;       // pointer to array of uint64_t's

    char arguments[256];        // command-line arguments passed to the kernel
} __attribute__((packed)) KernelBootInfo;

#define BOOT_FLAGS_UEFI     0x01
#define BOOT_FLAGS_GPT      0x02

typedef struct {
    uint64_t base;
    uint64_t len;
    uint32_t type;
    uint32_t acpiAttributes;
} __attribute__((packed)) MemoryMap;

#define MEMORY_TYPE_USABLE              1
#define MEMORY_TYPE_RESERVED            2
#define MEMORY_TYPE_ACPI_RECLAIMABLE    3
#define MEMORY_TYPE_ACPI_NVS            4
#define MEMORY_TYPE_BAD                 5

#define MEMORY_ATTRIBUTES_VALID         0x01
#define MEMORY_ATTRIBUTES_NV            0x02

