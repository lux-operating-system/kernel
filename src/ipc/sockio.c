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
    else p = getProcess(getPid());
    if(!p) return -ESRCH;

    if(!p->io[sd].valid || !p->io[sd].data || (p->io[sd].type != IO_SOCKET))
        return -ENOTSOCK;

    SocketDescriptor *self = (SocketDescriptor*) p->io[sd].data;
    SocketDescriptor *peer = self->peer;
    if(!peer) return -EDESTADDRREQ;     // not in connection mode

    sa_family_t family = self->address.sa_family;

    socketLock();

    if(family == AF_UNIX || family == AF_LOCAL) {
        // simply append to the peer's inbound list
        void **newlist = realloc(peer->inbound, (peer->inboundCount + 1) * sizeof(void *));
        if(!newlist) {
            socketRelease();
            return -ENOBUFS;
        }

        void *message = malloc(len);
        if(!message) {
            free(newlist);
            socketRelease();
            return -ENOBUFS;
        }

        // and send
        memcpy(message, buffer, len);
        newlist[peer->inboundCount] = message;
        peer->inboundCount++;
        peer->inbound = newlist;

        socketRelease();
        return len;
    } else {
        /* TODO: handle other protocols in user space */
        socketRelease();
        return -ENOTCONN;
    }
}
