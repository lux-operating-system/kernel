/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

/* Socket Connection Functions */
/* connect(), listen(), and accept() are implemented here */

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <platform/lock.h>
#include <kernel/logger.h>
#include <kernel/socket.h>
#include <kernel/io.h>
#include <kernel/sched.h>

/* connect(): creates a socket connection
 * params: t - calling thread, NULL for kernel threads
 * params: sd - socket descriptor
 * params: addr - peer address
 * params: len - length of peer address
 * returns: zero on success, negative error code on fail
 */

int connect(Thread *t, int sd, const struct sockaddr *addr, socklen_t len) {
    Process *p;
    if(t) p = getProcess(t->pid);
    else p = getProcess(getPid());
    if(!p) return -ESRCH;

    if(!p->io[sd].valid || !p->io[sd].data || (p->io[sd].type != IO_SOCKET))
        return -ENOTSOCK;

    SocketDescriptor *self = (SocketDescriptor *) p->io[sd].data;
    SocketDescriptor *peer = getLocalSocket(addr, len);

    if(!peer) return -EADDRNOTAVAIL;
    if(self->address.sa_family != peer->address.sa_family) return -EAFNOSUPPORT;
    if(!peer->listener || !peer->backlogMax || !peer->backlog) return -ECONNREFUSED;
    if(peer->backlogCount >= peer->backlogMax) return -ETIMEDOUT;

    // at this point we're sure it's safe to create a connection
    socketLock();
    peer->backlog[peer->backlogCount] = self;
    peer->backlogCount++;
    socketRelease();
    return 0;
}

/* listen(): listens for incoming connections on a socket
 * params: t - calling thread, NULL for kernel threads
 * params: sd - socket descriptor
 * params: backlog - maximum number of queued connections, zero for default
 * returns: zero on success, negative error code on fail
 */

int listen(Thread *t, int sd, int backlog) {
    Process *p;
    if(t) p = getProcess(t->pid);
    else p = getProcess(getPid());
    if(!p) return -ESRCH;

    if(!p->io[sd].valid || !p->io[sd].data || (p->io[sd].type != IO_SOCKET))
        return -ENOTSOCK;
    
    socketLock();
    SocketDescriptor *sock = (SocketDescriptor *) p->io[sd].data;
    sock->backlogCount = 0;

    if(backlog > 0) sock->backlogMax = backlog;
    else sock->backlogMax = SOCKET_DEFAULT_BACKLOG;
    
    sock->backlog = calloc(backlog, sizeof(SocketDescriptor *));
    if(!sock->backlog) {
        socketRelease();
        return -ENOBUFS;
    }

    sock->listener = true;
    socketRelease();
    return 0;
}
