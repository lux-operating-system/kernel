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
    else p = getProcess(getKernelPID());
    if(!p) return -ESRCH;

    if(!p->io[sd].valid || !p->io[sd].data || (p->io[sd].type != IO_SOCKET))
        return -ENOTSOCK;

    socketLock();

    SocketDescriptor *self = (SocketDescriptor *) p->io[sd].data;
    SocketDescriptor *peer = getLocalSocket(addr, len);

    if(!peer) {
        socketRelease();
        return -EADDRNOTAVAIL;
    }

    if(self->address.sa_family != peer->address.sa_family) {
        socketRelease();
        return -EAFNOSUPPORT;
    }

    if(!peer->listener || !peer->backlogMax || !peer->backlog) {
        socketRelease();
        return -ECONNREFUSED;
    }

    if(peer->backlogCount >= peer->backlogMax) {
        socketRelease();
        return -ETIMEDOUT;
    }

    // at this point we're sure it's safe to create a connection
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
    else p = getProcess(getKernelPID());
    if(!p) return -ESRCH;

    if(!p->io[sd].valid || !p->io[sd].data || (p->io[sd].type != IO_SOCKET))
        return -ENOTSOCK;
    
    socketLock();
    SocketDescriptor *sock = (SocketDescriptor *) p->io[sd].data;
    sock->backlogCount = 0;

    if(backlog > 0) sock->backlogMax = backlog;
    else sock->backlogMax = SOCKET_DEFAULT_BACKLOG;

    sock->backlog = calloc(sock->backlogMax, sizeof(SocketDescriptor *));
    if(!sock->backlog) {
        socketRelease();
        return -ENOBUFS;
    }

    sock->listener = true;
    socketRelease();
    return 0;
}

/* accept(): accepts an incoming socket connection
 * this function does NOT block at the kernel level - the syscall dispatcher
 * will take care of blocking if the socket is not set to be non-blocking
 * params: t - calling thread, NULL for kernel threads
 * params: sd - socket descriptor
 * params: addr - buffer to store peer's address
 * params: len - length of the buffer on input, length of data stored on output
 * returns: positive socket descriptor on success, negative error code on fail
 */

int accept(Thread *t, int sd, struct sockaddr *addr, socklen_t *len) {
    Process *p;
    if(t) p = getProcess(t->pid);
    else p = getProcess(getKernelPID());
    if(!p) return -ESRCH;

    if(!p->io[sd].valid || !p->io[sd].data || (p->io[sd].type != IO_SOCKET))
        return -ENOTSOCK;

    SocketDescriptor *listener = (SocketDescriptor *)p->io[sd].data;
    if(!listener->listener || !listener->backlog || !listener->backlogMax) {
        return -EINVAL;         // socket is not listening
    }

    if(!listener->backlogCount) {
        return -EWOULDBLOCK;    // socket has no incoming queue
    }

    // create a new connected socket
    IODescriptor *iod = NULL;
    int connectedSocket = openIO(p, (void **) &iod);
    if((connectedSocket < 0) || !iod) {
        return -EMFILE;
    }

    iod->type = IO_SOCKET;
    iod->flags = p->io[sd].flags;
    iod->data = calloc(1, sizeof(SocketDescriptor));
    if(!iod->data) {
        closeIO(p, iod);
        return -ENOMEM;
    }

    // copy the self address
    SocketDescriptor *self = (SocketDescriptor *)iod->data;
    self->refCount = 1;
    memcpy(&self->address, &listener->address, sizeof(struct sockaddr));
    self->type = listener->type;
    self->protocol = listener->protocol;
    self->process = listener->process;

    // and assign the peer address
    self->peer = listener->backlog[0];  // TODO: is this always FIFO?
    self->peer->peer = self;
    memmove(&listener->backlog[0], &listener->backlog[1], (listener->backlogMax - 1) * sizeof(SocketDescriptor *));
    listener->backlogCount--;

    // save the peer address if requested
    if(addr && len) {
        if(*len > sizeof(struct sockaddr)) *len = sizeof(struct sockaddr);
        memcpy(addr, &self->peer->address, *len);
    }

    if(!self->peer) {
        return -ECONNABORTED;
    } else {
        return connectedSocket;
    }
}
