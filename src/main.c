/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

#include <kernel/sched.h>

// the true kernel entry point is called after platform-specific initialization
// platform-specific code is in platform/[PLATFORM]/main.c

int main(int argc, char **argv) {
    schedInit();        // scheduler

    while(1);
}