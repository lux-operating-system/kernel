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

// per-CPU kernel information structure
// this will be stored using the kernel GS segment

typedef struct {
    int cpuIndex;
    PlatformCPU *cpu;
    uint64_t uptime;
    // more info will be added probably, but for now the kernel just needs to know
} KernelCPUInfo;

int smpBoot();
KernelCPUInfo *getKernelCPUInfo();

/* entry point for non-boot CPUs */

extern uint8_t apEntry[];
extern uint32_t apEntryVars[];

#define AP_ENTRY_SIZE           4096
#define AP_ENTRY_GDTR           1       // index into apEntryVars[]
#define AP_ENTRY_IDTR           2
#define AP_ENTRY_CR3            3
#define AP_ENTRY_STACK_LOW      4
#define AP_ENTRY_STACK_HIGH     5
#define AP_ENTRY_NEXT_LOW       6
#define AP_ENTRY_NEXT_HIGH      7

#define AP_STACK_SIZE           32768