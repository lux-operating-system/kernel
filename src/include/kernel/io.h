/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

/* Abstractions for file systems and sockets */

#include <stdint.h>
#include <stdbool.h>

// these are special and reserved descriptors
#define IO_STDIN                0
#define IO_STDOUT               1
#define IO_STDERR               2

#define IO_FILE                 3
#define IO_SOCKET               4

// TODO: decide whether to implement named pipes as files or an independent type

typedef struct {
    bool valid;
    int type;
    void *data;                 // file or socket-specific data
} IODescriptor;
