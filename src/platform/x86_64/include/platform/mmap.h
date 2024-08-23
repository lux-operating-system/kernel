/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Platform-Specific Code for x86_64
 */

#pragma once

// these constants must be defined for every CPU architecture
#define PAGE_SIZE               4096            // bytes
#define IDENTITY_MAP_GBS        16              // how many GiB of memory will be identity mapped
#define KERNEL_BASE_ADDRESS     0x200000        // 2 MiB
#define KERNEL_HEAP_BASE        (IDENTITY_MAP_GBS << 30)    // 16 GB
#define USER_BASE_ADDRESS       (IDENTITY_MAP_GBS << 31)    // 32 GB
