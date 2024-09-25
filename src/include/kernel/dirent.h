/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

#pragma once

#include <sys/types.h>

typedef struct {
    ino_t d_ino;
    char d_name[];
} dirent;
