/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <kernel/sched.h>
#include <sys/types.h>
#include <sys/stat.h>

/* system-wide limits */
#define MAX_FILE                (1 << 18)   // 262k
#define MAX_FILE_PATH           2048

/* file-specific I/O descriptor (see io.h) */
typedef struct {
    Process *process;
    char path[MAX_FILE_PATH];
    size_t position;
    struct stat info;
} FileDescriptor;

/* file system syscalls */
int mount(Thread *, uint64_t, const char *, const char *, const char *, int);
