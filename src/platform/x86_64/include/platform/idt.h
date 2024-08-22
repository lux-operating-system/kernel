/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Platform-Specific Code for x86_64
 */

#include <stdint.h>

#pragma once

#define IDT_FLAGS_VALID                 0x8000

#define IDT_FLAGS_TYPE_INTERRUPT        0x0E
#define IDT_FLAGS_TYPE_TRAP             0x0F
#define IDT_FLAGS_TYPE_SHIFT            8

#define IDT_FLAGS_DPL_KERNEL            0
#define IDT_FLAGS_DPL_USER              3
#define IDT_FLAGS_DPL_SHIFT             13

typedef struct {
    uint16_t offset_lo;
    uint16_t segment;
    uint16_t flags;
    uint16_t offset_mi;
    uint32_t offset_hi;
    uint32_t reserved;
} __attribute__((packed)) IDTEntry;

typedef struct {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) IDTR;
