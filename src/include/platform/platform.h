/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

#pragma once

#include <stdint.h>

/* routines that must be implemented by any platform-specific code, abstracting
 * the difference between different platforms; the goal is to keep the core
 * kernel portable across CPU architectures */

#define PLATFORM_PAGE_PRESENT               0x0001      // present in main memory
#define PLATFORM_PAGE_SWAP                  0x0002      // present in storage device
#define PLATFORM_PAGE_USER                  0x0004      // user-kernel toggle
#define PLATFORM_PAGE_EXEC                  0x0008
#define PLATFORM_PAGE_WRITE                 0x0010
#define PLATFORM_PAGE_ERROR                 0x8000

int platformCPUSetup();         // very early setup for one CPU
int platformPagingSetup();      // paging setup for virtual memory management
uintptr_t platformGetPage(int *, uintptr_t);     // get physical address and flags of a page
uintptr_t platformMapPage(uintptr_t, uintptr_t, int);    // map a physical address to a virtual address
int platformUnmapPage(uintptr_t);               // and vice versa

int platformRegisterCPU(void *);
int platformCountCPU();
void *platformGetCPU(int);
