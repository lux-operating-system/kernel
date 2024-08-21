/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <kernel/boot.h>

#define PAGE_SIZE               4096

#define PMM_CONTIGUOUS_LOW      0x01

typedef struct {
    uint64_t highestPhysicalAddress;
    uint64_t lowestUsableAddress;
    uint64_t highestUsableAddress;
    size_t highestPage;
    size_t usablePages, usedPages;
    size_t reservedPages;
} PhysicalMemoryStatus;

void pmmInit(KernelBootInfo *);
void pmmStatus(PhysicalMemoryStatus *);
uintptr_t pmmAllocate(void);
uintptr_t pmmAllocateContiguous(size_t, int);
int pmmFree(uintptr_t);
int pmmFreeContiguous(uintptr_t, size_t);