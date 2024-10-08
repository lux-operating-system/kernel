/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Platform-Specific Code for x86_64
 */

#include <stdint.h>

#pragma once

#define KENREL_STACK_SIZE           32768

typedef struct {
    uint32_t reserved1;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved2;
    uint64_t ist[7];
    uint64_t reserved3;
    uint16_t reserved4;
    uint16_t iomap;
    uint8_t ioports[8192];      // I/O port bitmap, 0 = allowed, 1 = deny
    uint8_t ones;               // final byte must be all ones
} __attribute__((packed)) TSS;

void tssSetup();
