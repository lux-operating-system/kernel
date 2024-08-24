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
