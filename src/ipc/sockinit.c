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
#include <kernel/socket.h>
#include <kernel/io.h>
#include <kernel/sched.h>

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

    IODescriptor *iod = NULL;       // open I/O descriptor
    int sd = openIO(p, (void **) &iod);
    if(sd < 0 || !iod) return sd;

    iod->type = IO_SOCKET;
    iod->data = calloc(1, sizeof(SocketDescriptor));
    if(!iod->data) {
        closeIO(p, iod);
    }

    // set up the socket family for now
    SocketDescriptor *sock = (SocketDescriptor *)iod->data;
    sock->address.sa_family = domain;
    sock->type = type;
    sock->protocol = protocol;

    return sd;
}
