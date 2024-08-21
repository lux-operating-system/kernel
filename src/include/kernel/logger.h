/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

#pragma once

#include <stdint.h>
#include <stdarg.h>

#define KPRINTF_LEVEL_DEBUG     0
#define KPRINTF_LEVEL_WARNING   1
#define KPRINTF_LEVEL_ERROR     2
#define KPRINTF_LEVEL_PANIC     3

#define KDEBUG(x)               kprintf(KPRINTF_LEVEL_DEBUG, 0, __FILE__+4, x)
#define KWARN(x)                kprintf(KPRINTF_LEVEL_WARNING, 0, __FILE__+4, x)
#define KERROR(x)               kprintf(KPRINTF_LEVEL_ERROR, 0, __FILE__+4, x)
#define KPANIC(x)               kprintf(KPRINTF_LEVEL_PANIC, 0, __FILE__+4, x)

int kprintf(int, uint64_t, const char *, const char *, ...);
int vkprintf(int, uint64_t, const char *, const char *, va_list);
