/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Platform-Specific Code for x86_64
 */

#include <stdint.h>

#pragma once

// 0x00: null descriptor
// 0x08: kernel code descriptor
// 0x10: kernel data descriptor
// 0x18: user code descriptor
// 0x20: user data descriptor
// 0x28: task segment descriptor

#define GDT_ENTRIES             7

#define GDT_NULL                0
#define GDT_KERNEL_CODE         1
#define GDT_KERNEL_DATA         2
#define GDT_USER_CODE           3
#define GDT_USER_DATA           4
#define GDT_TSS_LOW             5
#define GDT_TSS_HIGH            6

#define GDT_LIMIT               0xFFFFF     // constant for all entries
#define GDT_BASE                0

#define GDT_ACCESS_ACCESSED     0x01
#define GDT_ACCESS_RW           0x02
#define GDT_ACCESS_DC           0x04        // direction (data) or conforming (code)
#define GDT_ACCESS_EXEC         0x08
#define GDT_ACCESS_CODE_DATA    0x10
#define GDT_ACCESS_PRESENT      0x80

#define GDT_ACCESS_DPL_SHIFT    5           // privilege level
#define GDT_ACCESS_DPL_KERNEL   0
#define GDT_ACCESS_DPL_USER     3

#define GDT_FLAGS_64_BIT        0x20
#define GDT_FLAGS_32_BIT        0x40
#define GDT_FLAGS_PAGE_GRAN     0x80

typedef struct {
    uint16_t limit;
    uint16_t baseLo;
    uint8_t baseMi;
    uint8_t access;
    uint8_t flagsLimitHi;
    uint8_t baseHi;
} __attribute__((packed)) GDTEntry;

typedef struct {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) GDTR;
