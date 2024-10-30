/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

/* Socket I/O Functions */
/* send() and recv() are implemented here */

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <kernel/logger.h>
#include <kernel/socket.h>
#include <kernel/io.h>
#include <kernel/sched.h>

/* send(): sends a message to a socket connection
 * params: t - calling thread
 * params: sd - socket descriptor
 * params: buffer - buffer containing the message
 * params: len - size of the message
 * params: flags - optional flags for the request
 * returns: positive number of bytes sent, negative error code on fail
 */

ssize_t send(Thread *t, int sd, const void *buffer, size_t len, int flags) {
    Process *p;
    if(t) p = getProcess(t->pid);
    else p = getProcess(getKernelPID());
    if(!p) return -ESRCH;

    if(!p->io[sd].valid || !p->io[sd].data || (p->io[sd].type != IO_SOCKET))
        return -ENOTSOCK;

    SocketDescriptor *self = (SocketDescriptor*) p->io[sd].data;
    SocketDescriptor *peer = self->peer;
    if(!peer) return -EDESTADDRREQ;     // not in connection mode

    acquireLockBlocking(&peer->lock);

    sa_family_t family = self->address.sa_family;

    if(family == AF_UNIX || family == AF_LOCAL) {
        if(!peer->inbound || !peer->inboundLen || !peer->inboundMax) {
            // ensure that the peer's inbound list exists at all
            peer->inbound = calloc(SOCKET_IO_BACKLOG, sizeof(void *));
            peer->inboundLen = calloc(SOCKET_IO_BACKLOG, sizeof(size_t));

            if(!peer->inbound || !peer->inboundLen) {
                releaseLock(&peer->lock);
                return -ENOMEM;
            }

            peer->inboundMax = SOCKET_IO_BACKLOG;
            peer->inboundCount = 0;
        }

        if(peer->inboundCount >= peer->inboundMax) {
            // reallocate the backlog if necessary
            void **newlist = realloc(peer->inbound, peer->inboundMax * 2 * sizeof(void *));
            if(!newlist) {
                releaseLock(&peer->lock);
                return -ENOMEM;
            }

            peer->inbound = newlist;

            size_t *newlen = realloc(peer->inboundLen, peer->inboundMax * 2 * sizeof(size_t));
            if(!newlen) {
                releaseLock(&peer->lock);
                return -ENOMEM;
            }

            peer->inboundLen = newlen;
            peer->inboundMax *= 2;
        }

        void *message = malloc(len);
        if(!message) {
            releaseLock(&peer->lock);
            return -ENOBUFS;
        }

        // and send
        memcpy(message, buffer, len);
        peer->inbound[peer->inboundCount] = message;
        peer->inboundLen[peer->inboundCount] = len;
        peer->inboundCount++;

        releaseLock(&peer->lock);
        return len;
    } else {
        /* TODO: handle other protocols in user space */
        releaseLock(&peer->lock);
        return -ENOTCONN;
    }
}

/* recv(): receives a message from a socket connection
 * params: t - calling thread
 * params: sd - socket descriptor
 * params: buffer - buffer to store message
 * params: len - maximum size of the buffer
 * params: flags - optional flags for the request
 * returns: positive number of bytes received, negative error code on fail
 */

ssize_t recv(Thread *t, int sd, void *buffer, size_t len, int flags) {
    Process *p;
    if(t) p = getProcess(t->pid);
    else p = getProcess(getKernelPID());
    if(!p) return -ESRCH;

    if(!p->io[sd].valid || !p->io[sd].data || (p->io[sd].type != IO_SOCKET))
        return -ENOTSOCK;

    SocketDescriptor *self = (SocketDescriptor*) p->io[sd].data;
    if(!self->peer) return -EDESTADDRREQ;   // not in connection mode

    acquireLockBlocking(&self->lock);

    sa_family_t family = self->address.sa_family;
    if(!self->inboundCount || !self->inbound || !self->inboundLen) {
        releaseLock(&self->lock);
        return -EWOULDBLOCK;    // no messages available
    }

    if(family == AF_UNIX || family == AF_LOCAL) {
        // copy from the inbound list
        void *message = self->inbound[0];   // FIFO
        size_t truelen = self->inboundLen[0];

        if(!message) {
            releaseLock(&self->lock);
            return -EWOULDBLOCK;
        }

        if(truelen > len) truelen = len;    // truncate longer messages
        memcpy(buffer, message, truelen);

        // remove the received message from the queue if we're in non-peek mode
        if(!(flags & MSG_PEEK)) {
            free(message);

            self->inboundCount--;
            if(self->inboundCount) {
                memcpy(&self->inbound[0], &self->inbound[1], self->inboundCount * sizeof(void *));
                memcpy(&self->inboundLen[0], &self->inboundLen[1], self->inboundCount * sizeof(size_t));
            }
        }

        releaseLock(&self->lock);
        return truelen;
    } else {
        /* TODO: handle other protocols in user space */
        releaseLock(&self->lock);
        return -ENOTCONN;
    }
}