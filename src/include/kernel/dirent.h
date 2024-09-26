/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

#pragma once

#include <sys/types.h>
#include <kernel/sched.h>
#include <kernel/file.h>

// directory descriptors will be OR'ed with this flag
#define DIRECTORY_DESCRIPTOR_FLAG       0x8000

typedef int DIR;

struct dirent {
    ino_t d_ino;
    char d_name[];
};

/* directory-specific I/O descriptor (see io.h) */
typedef struct {
    Process *process;
    char path[MAX_FILE_PATH];
    char device[MAX_FILE_PATH];
    size_t position;
} DirectoryDescriptor;

int opendir(Thread *, uint64_t, const char *);
int closedir(Thread *, DIR *);
int readdir_r(Thread *, uint64_t, DIR *, struct dirent *, struct dirent **);
void seekdir(Thread *, DIR *, long);
long telldir(Thread *, DIR *);
