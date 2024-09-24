/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Platform-Specific Code for x86_64
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <kernel/boot.h>
#include <kernel/memory.h>
#include <kernel/tty.h>
#include <kernel/acpi.h>
#include <kernel/modules.h>
#include <kernel/logger.h>
#include <kernel/version.h>
#include <platform/platform.h>
#include <platform/x86_64.h>
#include <platform/exception.h>
#include <platform/apic.h>

extern int main(int, char **);
KernelBootInfo boot;

// x86_64-specific kernel entry point
int platformMain(KernelBootInfo *k) {
    memcpy(&boot, k, sizeof(KernelBootInfo));

    // check if the kernel is booting is quiet mode
    size_t len = strlen(boot.arguments);
    if(len >= 9) {
        for(size_t i = 0; i <= len-6; i++) {
            if(!memcmp(boot.arguments+i, " quiet", 6)) {
                loggerSetVerbose(false);
                break;
            }
        }
    }

    platformCPUSetup();
    ttyInit(&boot);

    KDEBUG("%s\n", KERNEL_VERSION);
    KDEBUG("booting with command-line options: %s\n", boot.arguments);

    installExceptions();
    pmmInit(&boot);
    vmmInit();

    ttyCreateBackbuffer();
    acpiInit(&boot);
    apicInit();
    platformInitialSeed();
    ramdiskInit(&boot);
    modulesInit(&boot);

    char **argv;
    int argc = parseBootArgs(&argv, boot.arguments);
    return main(argc, argv);
}