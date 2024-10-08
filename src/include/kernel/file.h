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

#define SEEK_SET                1
#define SEEK_CUR                2
#define SEEK_END                3

/* file-specific I/O descriptor (see io.h) */
typedef struct {
    Process *process;
    char abspath[MAX_FILE_PATH];    // absolute path
    char device[MAX_FILE_PATH];     // device
    char path[MAX_FILE_PATH];       // path relative to device mountpount
    off_t position;
    uint64_t id;                    // unique ID, this is for device files
} FileDescriptor;

/* file system syscalls */
int open(Thread *, uint64_t, const char *, int, mode_t);
int close(Thread *, int);
ssize_t read(Thread *, uint64_t, int, void *, size_t);
ssize_t write(Thread *, uint64_t, int, const void *, size_t);
off_t lseek(Thread *, int, off_t, int);
int mount(Thread *, uint64_t, const char *, const char *, const char *, int);

ssize_t readFile(Thread *, uint64_t, IODescriptor *, void *, size_t);
ssize_t writeFile(Thread *, uint64_t, IODescriptor *, const void *, size_t);
int closeFile(Thread *, int);

int chdir(Thread *, uint16_t, const char *);
char *getcwd(Thread *, char *, size_t);
