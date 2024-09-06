/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

/* Socket Initialization Functions */
/* socket(), bind(), and close() for sockets are implemented here */

/* I tried my best to follow The Base Specification Issue 8 */

#include <errno.h>
#include <stdlib.h>
#include <platform/lock.h>
#include <kernel/logger.h>
#include <kernel/socket.h>
#include <kernel/io.h>
#include <kernel/sched.h>

/* array of system-wide open sockets */
static lock_t lock = LOCK_INITIAL;
static SocketDescriptor *sockets;
static int socketCount;

/* socketInit(): initializes the socket subsystem
 * params: none
 * returns: nothing
 */

void socketInit() {
    sockets = calloc(sizeof(SocketDescriptor *), MAX_SOCKETS);
    if(!sockets) {
        KERROR("failed to allocate memory for socket subsystem\n");
        while(1);
    }

    socketCount = 0;

    KDEBUG("max %d sockets, %d per process\n", MAX_SOCKETS, MAX_IO_DESCRIPTORS);
}

/* socket(): opens a communication socket
 * params: t - calling thread, NULL for kernel threads
 * params: domain - socket domain/family
 * params: type - type of socket (connection, datagram, etc)
 * params: protocol - protocol implementing "type" on "domain", zero for default
 * returns: positive socket descriptor on success, negative error code on fail
 */

int socket(Thread *t, int domain, int type, int protocol) {
    Process *p;
    if(t) p = getProcess(t->pid);
    else p = getProcess(getPid());

    if(!p) return -ESRCH;
    if(p->iodCount == MAX_IO_DESCRIPTORS) return -EMFILE;

    acquireLockBlocking(&lock);

    IODescriptor *iod = NULL;       // open I/O descriptor
    int sd = openIO(p, (void **) &iod);
    if(sd < 0 || !iod) return sd;

    iod->type = IO_SOCKET;
    iod->data = calloc(1, sizeof(SocketDescriptor));
    if(!iod->data) {
        releaseLock(&lock);
        closeIO(p, iod);
    }

    // set up the socket family for now
    SocketDescriptor *sock = (SocketDescriptor *)iod->data;
    sock->address.sa_family = domain;
    sock->type = type;
    sock->protocol = protocol;

    sockets[socketCount] = sock;
    socketCount++;

    releaseLock(&lock);
    return sd;
}
