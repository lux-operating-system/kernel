/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

#pragma once

/* this is identical to the structures used in the boot loader */

#include <stdint.h>

#define ELF_VERSION                 1

/* general header for ELF files */
typedef struct {
    // this structure follows the 64-bit ELF format because there's no need
    // to implement 32-bit ELF
    uint8_t magic[4];       // 0x7F, 'ELF'
    uint8_t isaWidth;       // 2 = 64-bit
    uint8_t endianness;     // 1 = little endian
    uint8_t headerVersion;
    uint8_t abi;            // 0 = System V
    uint64_t reserved;

    uint16_t type;
    uint16_t isa;           // CPU architecture
    uint32_t version;
    uint64_t entryPoint;    // absolute address in virtual memory, not the ELF file structure
    uint64_t headerTable;
    uint64_t sectionTable;
    uint32_t flags;         // architecture-dependent; undefined for x86/x86_64

    uint16_t headerSize;
    uint16_t headerEntrySize;
    uint16_t headerEntryCount;
    uint16_t sectionEntrySize;
    uint16_t sectionEntryCount;
    uint16_t sectionHeaderStrings;
} __attribute__((packed)) ELFFileHeader;

#define ELF_ISA_WIDTH_64            2
#define ELF_LITTLE_ENDIAN           1
#define ELF_SYSV_ABI                0

#define ELF_TYPE_RELOC              1
#define ELF_TYPE_EXEC               2
#define ELF_TYPE_SHARED             3
#define ELF_TYPE_CORE               4

#define ELF_ARCHITECTURE_X86_64     0x3E
#define ELF_ARCHITECTURE_RISCV      0xF3
#define ELF_ARCHITECTURE_ARM64      0xB7

/* these will be set by the compiler */
#ifdef PLATFORM_X86_64
#define ELF_ARCHITECTURE            ELF_ARCHITECTURE_X86_64
#endif

#ifdef PLATFORM_RISCV
#define ELF_ARCHITECTURE            ELF_ARCHITECTURE_RISCV
#endif

#ifdef PLATFORM_ARM64
#define ELF_ARCHITECTURE            ELF_ARCHITECTURE_ARM64
#endif

/* program header, pointed to by headerTable */
typedef struct {
    uint32_t segmentType;
    uint32_t flags;
    uint64_t fileOffset;
    uint64_t virtualAddress;
    uint64_t physicalAddress;
    uint64_t fileSize;
    uint64_t memorySize;
    uint64_t alignment;
} __attribute__((packed)) ELFProgramHeader;

#define ELF_SEGMENT_TYPE_NULL       0
#define ELF_SEGMENT_TYPE_LOAD       1
#define ELF_SEGMENT_TYPE_DYNAMIC    2
#define ELF_SEGMENT_TYPE_INTERPRET  3
#define ELF_SEGMENT_TYPE_NOTES      4

#define ELF_SEGMENT_FLAGS_EXEC      0x01
#define ELF_SEGMENT_FLAGS_WRITE     0x02
#define ELF_SEGMENT_FLAGS_READ      0x04

uint64_t loadELF(const void *, uint64_t *);
