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
    
    socketLock();

    SocketDescriptor *self = (SocketDescriptor*) p->io[sd].data;
    SocketDescriptor *peer = self->peer;
    if(!peer) {
        socketRelease();
        return -EDESTADDRREQ;     // not in connection mode
    }

    sa_family_t family = self->address.sa_family;

    if(family == AF_UNIX || family == AF_LOCAL) {
        // simply append to the peer's inbound list
        void **newlist = realloc(peer->inbound, (peer->inboundCount+1) * sizeof(void *));
        if(!newlist) {
            socketRelease();
            return -ENOBUFS;
        }

        size_t *newlen = realloc(peer->inboundLen, (peer->inboundCount+1) * sizeof(size_t));

        if(!newlen) {
            free(newlist);
            socketRelease();
            return -ENOBUFS;
        }

        void *message = malloc(len);
        if(!message) {
            free(newlist);
            free(newlen);
            socketRelease();
            return -ENOBUFS;
        }

        // and send
        memcpy(message, buffer, len);
        newlist[peer->inboundCount] = message;
        newlen[peer->inboundCount] = len;
        peer->inboundCount++;
        peer->inbound = newlist;
        peer->inboundLen = newlen;

        socketRelease();
        return len;
    } else {
        /* TODO: handle other protocols in user space */
        socketRelease();
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

    socketLock();

    SocketDescriptor *self = (SocketDescriptor*) p->io[sd].data;
    if(!self->peer) {
        socketRelease();
        return -EDESTADDRREQ;   // not in connection mode
    }

    sa_family_t family = self->address.sa_family;
    if(!self->inboundCount || !self->inbound || !self->inboundLen) {
        socketRelease();
        return -EWOULDBLOCK;    // no messages available
    }

    if(family == AF_UNIX || family == AF_LOCAL) {
        // copy from the inbound list
        void *message = self->inbound[0];   // FIFO
        size_t truelen = self->inboundLen[0];

        if(!message) {
            socketRelease();
            return -EWOULDBLOCK;
        }

        if(truelen > len) truelen = len;    // truncate longer messages
        memcpy(buffer, message, truelen);

        // and remove the received message from the queue
        free(message);

        self->inboundCount--;
        if(!self->inboundCount) {
            free(self->inbound);
            free(self->inboundLen);
            self->inbound = NULL;
            self->inboundLen = NULL;
        } else {
            memmove(&self->inbound[0], &self->inbound[1], self->inboundCount * sizeof(void *));
            memmove(&self->inboundLen[0], &self->inboundLen[1], self->inboundCount * sizeof(size_t));
            self->inbound = realloc(self->inbound, self->inboundCount * sizeof(void *));
            self->inboundLen = realloc(self->inboundLen, self->inboundCount * sizeof(void *));
        }

        socketRelease();
        return truelen;
    } else {
        /* TODO: handle other protocols in user space */
        socketRelease();
        return -ENOTCONN;
    }
}