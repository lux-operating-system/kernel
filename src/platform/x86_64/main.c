/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Platform-Specific Code for x86_64
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <kernel/boot.h>
#include <kernel/memory.h>
#include <kernel/tty.h>
#include <kernel/acpi.h>
#include <kernel/modules.h>
#include <platform/platform.h>
#include <platform/x86_64.h>
#include <platform/exception.h>
#include <platform/apic.h>

extern int main(int, char **);
KernelBootInfo boot;

// x86_64-specific kernel entry point
int platformMain(KernelBootInfo *k) {
    memcpy(&boot, k, sizeof(KernelBootInfo));
    platformCPUSetup();
    ttyInit(&boot);
    installExceptions();
    pmmInit(&boot);
    vmmInit();
    acpiInit(&boot);
    apicInit();
    platformInitialSeed();
    ramdiskInit(&boot);
    modulesInit(&boot);

    char **argv;
    int argc = parseBootArgs(&argv, boot.arguments);
    return main(argc, argv);
}