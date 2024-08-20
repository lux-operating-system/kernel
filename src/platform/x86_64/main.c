/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

#include <stdint.h>
#include <boot.h>

// platform-specific kernel entry point
int platformMain(KernelBootInfo *k) {
    while(1);
    return 0;
}