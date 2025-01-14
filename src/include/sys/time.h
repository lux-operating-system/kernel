/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

#pragma once

#include <sys/types.h>
#include <kernel/sched.h>

struct timeval {
    time_t tv_sec;
    suseconds_t tv_usec;
};

int gettimeofday(Thread *, struct timeval *, void *);
