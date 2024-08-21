/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

#include <stddef.h>
#include <stdint.h>
#include <kernel/boot.h>
#include <kernel/tty.h>
#include <platform/platform.h>

extern int main(int, char **);

// x86_64-specific kernel entry point
int platformMain(KernelBootInfo *k) {
    platformCPUSetup();
    ttyInit(k);
    return main(0, NULL);
}