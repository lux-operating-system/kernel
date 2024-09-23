/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

/* Abstractions for file systems and sockets */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <kernel/sched.h>

#define MAX_IO_DESCRIPTORS      1024    // max files/sockets open per process

// these are special and reserved descriptors
#define IO_STDIN                0
#define IO_STDOUT               1
#define IO_STDERR               2

#define IO_WAITING              3   // only used during setup
#define IO_FILE                 4
#define IO_SOCKET               5

/* I/O descriptor flags */
#define O_NONBLOCK              0x0001
#define O_NDELAY                O_NONBLOCK
#define O_CLOEXEC               0x0002
#define O_RDONLY                0x0004
#define O_WRONLY                0x0008
#define O_RDWR                  (O_RDONLY | O_WRONLY)
#define O_APPEND                0x0010
#define O_CREAT                 0x0020
#define O_DSYNC                 0x0040
#define O_EXCL                  0x0080
#define O_NOCTTY                0x0100
#define O_RSYNC                 0x0200
#define O_SYNC                  0x0400
#define O_TRUNC                 0x0800

/* Reserved bits in ioctl() opcodes */
#define IOCTL_IN_PARAM          0x0001  /* third param is a value */
#define IOCTL_OUT_PARAM         0x0002  /* third param is a pointer */
#define IOCTL_RESERVED          0x000F  /* reserve the entire lowest 8 bits */

// TODO: decide whether to implement named pipes as files or an independent type

typedef struct IODescriptor {
    bool valid;
    int type;
    uint16_t flags;
    void *data;                 // file or socket-specific data
} IODescriptor;

int openIO(void *, void **);
void closeIO(void *, void *);
int ioperm(struct Thread *, uintptr_t, uintptr_t, int);
int ioctl(struct Thread *, uint64_t, int, unsigned long, ...);