/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

#include <stdlib.h>
#include <kernel/logger.h>
#include <kernel/sched.h>
#include <kernel/modules.h>
#include <platform/platform.h>

void *kernelThread(void *args) {
    KDEBUG("kernel thread with PID %d\n", getPid());

    // spawn the router in user space
    int64_t size = ramdiskFileSize("lumen");
    if(size <= 9) {
        KERROR("lumen not present on the ramdisk, halting because there's nothing to do\n");
        while(1) platformHalt();
    }

    void *lumen = malloc(size);
    if(!lumen) {
        KERROR("failed to allocate memory for lumen, halting because there's nothing to do\n");
        while(1) platformHalt();
    }

    if(ramdiskRead(lumen, "lumen", size) != size) {
        KERROR("failed to read lumen into memory, halting because there's nothing to do\n");
        while(1) platformHalt();
    }

    // TODO: maybe pass boot arguments to lumen?
    execveMemory(lumen, NULL, NULL);
    free(lumen);
    while(1);
}

// the true kernel entry point is called after platform-specific initialization
// platform-specific code is in platform/[PLATFORM]/main.c

int main(int argc, char **argv) {
    schedInit();        // scheduler

    threadCreate(&kernelThread, NULL);

    while(1) {
        platformHalt();
    }
}