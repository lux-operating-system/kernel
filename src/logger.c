/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

#include <stdio.h>
#include <kernel/logger.h>
#include <platform/lock.h>

static lock_t lock = LOCK_INITIAL;

int kprintf(int level, uint64_t timestamp, const char *src, const char *f, ...) {
    acquireLockBlocking(&lock);

    int len = printf("\e[37m%08d ", timestamp);

    switch(level) {
    case KPRINTF_LEVEL_DEBUG:
        len += printf("\e[32m");
        break;
    case KPRINTF_LEVEL_WARNING:
        len += printf("\e[33m");
        break;
    case KPRINTF_LEVEL_ERROR:
    case KPRINTF_LEVEL_PANIC:
    default:
        len += printf("\e[31m");
    }

    len += printf("%s: \e[37m", src);
    
    va_list args;
    va_start(args, f);
    len += vprintf(f, args);
    va_end(args);

    releaseLock(&lock);
    return len;
}
