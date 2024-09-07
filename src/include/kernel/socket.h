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

/* system-wide limit */
#define MAX_SOCKETS             (1 << 18)   // 262k

/* socket family/domain - only Unix sockets will be implemented in the kernel */
#define AF_UNIX                 1
#define AF_LOCAL                AF_UNIX

/* socket type - these will be ignored for local Unix sockets */
/* the kernel will ensure packets are sent and received in the same order */
#define SOCK_STREAM             1       // stream-oriented
#define SOCK_DGRAM              2       // datagram-oriented
#define SOCK_SEQPACKET          3       // connection-oriented

/* additional socket flags */
#define SOCK_NONBLOCK           0x100
#define SOCK_CLOEXEC            0x200

typedef uint16_t sa_family_t;
typedef size_t socklen_t;

/* generic socket */
struct sockaddr {
    sa_family_t sa_family;
    char sa_data[512];
};

/* Unix domain socket */
struct sockaddr_un {
    sa_family_t sun_family;     // AF_UNIX
    char sun_path[512];         // filename
};

/* socket-specific I/O descriptor (see io.h) */
typedef struct SocketDescriptor {
    Process *process;
    struct sockaddr address;
    bool listener;
    int type, protocol, backlog;
    int inboundCount, outboundCount;
    void **inbound, **outbound;
    struct SocketDescriptor *peer;
} SocketDescriptor;

void socketInit();
SocketDescriptor *getLocalSocket(const struct sockaddr *, socklen_t);
void socketLock();
void socketRelease();

/* socket system calls */
int socket(Thread *, int, int, int);
int connect(Thread *, int, const struct sockaddr *, socklen_t);
int bind(Thread *, int, const struct sockaddr *, socklen_t);
int listen(Thread *, int, int);
int accept(Thread *, int, struct sockaddr *, socklen_t *);
ssize_t recv(Thread *, int, void *, size_t, int);
ssize_t send(Thread *, int, const void *, size_t, int);
