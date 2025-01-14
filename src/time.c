/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

#include <platform/platform.h>
#include <kernel/sched.h>
#include <sys/time.h>

int gettimeofday(Thread *t, struct timeval *tv, void *tzp) {
    tv->tv_sec = platformTimestamp();
    tv->tv_usec = (platformUptime() % PLATFORM_TIMER_FREQUENCY) * 1000;
    return 0;
}
