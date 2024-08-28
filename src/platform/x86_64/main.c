/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Platform-Specific Code for x86_64
 */

#include <stddef.h>
#include <stdint.h>
#include <kernel/boot.h>
#include <kernel/memory.h>
#include <kernel/tty.h>
#include <kernel/acpi.h>
#include <platform/platform.h>
#include <platform/x86_64.h>
#include <platform/exception.h>
#include <platform/apic.h>

extern int main(int, char **);

// x86_64-specific kernel entry point
int platformMain(KernelBootInfo *k) {
    platformCPUSetup();
    ttyInit(k);
    installExceptions();
    pmmInit(k);
    vmmInit();
    acpiInit(k);
    apicInit();
    platformInitialSeed();

    char **argv;
    int argc = parseBootArgs(&argv, k->arguments);
    return main(argc, argv);
}