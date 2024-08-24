/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Platform-Specific Code for x86_64
 */

#pragma once

#include <stdint.h>

/* every platform must provide some implementation of locks */
#define LOCK_INITIAL            0       // initial free value

typedef uint64_t lock_t;
int lockStatus(lock_t *);
int acquireLock(lock_t *);
int acquireLockBlocking(lock_t *);
int releaseLock(lock_t *);
