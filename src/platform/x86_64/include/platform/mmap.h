/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Platform-Specific Code for x86_64
 */

#pragma once

// these constants must be defined for every CPU architecture
#define PAGE_SIZE               4096            // bytes
#define IDENTITY_MAP_GBS        1               // how many GiB of LOW memory will be identity mapped
#define KERNEL_BASE_ADDRESS     0x200000        // 2 MiB
#define KERNEL_HEAP_BASE        0x400000000000  // 64 TiB
#define KERNEL_HEAP_LIMIT       0x4FFFFFFFF000  // ~16 TiB of kernel heap 
#define KERNEL_MMIO_BASE        0x600000000000  // 96 TiB
#define KERNEL_MMIO_GBS         8               // 8 GB will be mapped at MMIO base
#define KERNEL_MMIO_LIMIT       ((uint64_t)KERNEL_MMIO_GBS << 30)
#define USER_BASE_ADDRESS       0x40000000      // 1 GB, user programs will be loaded here
#define USER_LIMIT_ADDRESS      0x3FFFFFFFF000  // ~64 TiB, this leaves user programs almost 64 TiB
