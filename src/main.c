/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

#include <kernel/logger.h>
#include <kernel/sched.h>
#include <platform/platform.h>

void *kernelThread(void *args) {
    KDEBUG("hello world from the kernel thread\n");
    KDEBUG("my process ID is %d\n", getPid());
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