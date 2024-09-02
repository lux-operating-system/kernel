/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Platform-Specific Code for x86_64
 */

#pragma once

#include <stdint.h>

// these constants must be defined for every CPU architecture
#define PAGE_SIZE               4096                            // bytes
#define KERNEL_BASE_ADDRESS     (uintptr_t)0xFFFF800000000000
#define KERNEL_MMIO_BASE        KERNEL_BASE_ADDRESS
#define KERNEL_BASE_MAPPED      16                              // gigabytes to be mapped
#define KERNEL_BASE_END         (KERNEL_BASE_ADDRESS-KERNEL_MMIO_LIMIT-1)
#define KERNEL_HEAP_BASE        (uintptr_t)0xFFFF900000000000
#define KERNEL_HEAP_LIMIT       (uintptr_t)0xFFFF9FFFFFFFFFFF
#define KERNEL_MMIO_LIMIT       ((uint64_t)KERNEL_BASE_MAPPED << 30)
#define USER_BASE_ADDRESS       0x400000                        // 4 MB, user programs will be loaded here
#define USER_LIMIT_ADDRESS      (KERNEL_BASE_ADDRESS-1)         // maximum limit for the lower half
