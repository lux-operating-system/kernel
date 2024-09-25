/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

#pragma once

#include <stdint.h>
#include <stddef.h>

// Unix system types

typedef uint64_t dev_t;     // device
typedef uint64_t ino_t;     // inode
typedef uint32_t mode_t;
typedef uint64_t nlink_t;
typedef uint32_t uid_t;
typedef uint32_t gid_t;
typedef int32_t pid_t;
typedef int64_t off_t;
typedef uint64_t time_t;
typedef uint16_t blksize_t;
typedef uint64_t blkcnt_t;
typedef int64_t ssize_t;
