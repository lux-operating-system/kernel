/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

#include <stdbool.h>
#include <stdio.h>
#include <kernel/logger.h>
#include <platform/lock.h>
#include <platform/platform.h>

static lock_t lock = LOCK_INITIAL;
static bool verbose = true;

void loggerSetVerbose(bool v) {
    verbose = v;
}

int kprintf(int level, const char *src, const char *f, ...) {
    if(!verbose && (level != KPRINTF_LEVEL_ERROR || level != KPRINTF_LEVEL_PANIC))
        return 0;

    acquireLockBlocking(&lock);

    int len = printf("\e[37m%08d ", platformUptime());

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
