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
typedef int32_t id_t;
typedef id_t uid_t;
typedef id_t gid_t;
typedef id_t pid_t;
typedef int64_t off_t;
typedef int64_t time_t;
typedef int64_t clock_t;
typedef int64_t suseconds_t;
typedef uint64_t useconds_t;
typedef int16_t blksize_t;
typedef int64_t blkcnt_t;
typedef int64_t ssize_t;
typedef int16_t clockid_t;
typedef uint64_t fsblkcnt_t;
typedef uint64_t fsfilcnt_t;
typedef int64_t timer_t;
