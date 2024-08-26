/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

#include <stdbool.h>
#include <platform/lock.h>
#include <kernel/sched.h>
#include <kernel/logger.h>

static bool scheduling = false;
static int processes = 0;
static int threads = 0;

/* schedInit(): initializes the scheduler */

void schedInit() {
    scheduling = true;
    processes = 0;
    threads = 0;
    KDEBUG("scheduler enabled\n");
}
