/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Platform-Specific Code for x86_64
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

// every platform must define some kind of structure that allows it to identify
// different CPUs in a multiprocessing system
// the contents of the structure are platform-dependent

typedef struct PlatformCPU {
    uint8_t procID, apicID;
    bool bootCPU;        // true for the BSP
    bool running;
    struct PlatformCPU *next;
} PlatformCPU;

int smpBoot();

/* entry point for non-boot CPUs */

extern uint8_t apEntry[];
extern uint32_t apEntryVars[];

#define AP_ENTRY_SIZE           4096
#define AP_ENTRY_GDTR           0       // index into apEntryVars[]
#define AP_ENTRY_IDTR           1
#define AP_ENTRY_CR3            2
#define AP_ENTRY_STACK          3
