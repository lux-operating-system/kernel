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

typedef struct {
    uint64_t highestPhysicalAddress;
    size_t highestPage;
    size_t usablePages, usedPages;
    size_t reservedPages;
} PhysicalMemoryStatus;

void pmmInit(KernelBootInfo *);
void pmmStatus(PhysicalMemoryStatus *);
uintptr_t pmmAllocate();
uintptr_t pmmAllocateContiguous(size_t);
void pmmFree(uintptr_t, size_t);
