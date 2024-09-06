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

// TODO: decide whether to implement named pipes as files or an independent type

typedef struct IODescriptor {
    bool valid;
    int type;
    void *data;                 // file or socket-specific data
} IODescriptor;

int openIO(void *, void **);
void closeIO(void *, void *);
