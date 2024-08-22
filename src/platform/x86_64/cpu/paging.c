/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Platform-Specific Code for x86_64
 */

#include <platform/x86_64.h>
#include <platform/platform.h>
#include <kernel/memory.h>

/* platformPagingSetup(): sets up the kernel's paging structures
 * this is called by the virtual memory manager early in the boot process
 */

int platformPagingSetup() {
    return 1;
}