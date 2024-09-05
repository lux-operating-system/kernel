/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

#pragma once

#include <stdint.h>
#include <stddef.h>

/* socket family/domain - only Unix sockets will be implemented in the kernel */
#define AF_UNIX                 1
#define AF_LOCAL                AF_UNIX

/* socket type - these will all be the same for the local Unix sockets */
/* the kernel will ensure packets are sent and received in the same order */
#define SOCK_STREAM             1       // stream-oriented
#define SOCK_DGRAM              2       // datagram-oriented
#define SOCK_SEQPACKET          3       // connection-oriented

typedef uint32_t sa_family_t;
typedef size_t socklen_t;

/* generic socket */
struct sockaddr {
    sa_family_t sa_family;
    char sa_data[];
}

/* Unix domain socket */
struct sockaddr_un {
    sa_family_t sun_family;     // AF_UNIX
    char sun_path[512];         // filename
}