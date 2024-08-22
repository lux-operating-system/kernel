/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

#include <stddef.h>
#include <stdint.h>
#include <kernel/boot.h>
#include <kernel/memory.h>
#include <kernel/tty.h>
#include <platform/platform.h>
#include <platform/x86_64.h>
#include <platform/exception.h>

extern int main(int, char **);

// x86_64-specific kernel entry point
int platformMain(KernelBootInfo *k) {
    platformCPUSetup();
    ttyInit(k);
    installExceptions();
    pmmInit(k);

    return main(0, NULL);
}