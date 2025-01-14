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
#include <string.h>
#include <platform/lock.h>
#include <kernel/logger.h>
#include <kernel/socket.h>
#include <kernel/io.h>
#include <kernel/sched.h>

/* array of system-wide open sockets */
static lock_t lock = LOCK_INITIAL;
static SocketDescriptor **sockets;
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

/* getLocalSocket(): finds a local socket by address
 * params: addr - socket address
 * params: len - length of socket address
 * returns: pointer to socket descriptor, NULL on fail
 */

SocketDescriptor *getLocalSocket(const struct sockaddr *addr, socklen_t len) {
    if(!socketCount) return NULL;
    if(len > sizeof(struct sockaddr)) len = sizeof(struct sockaddr);
    for(int i = 0; i < socketCount; i++) {
        if((sockets[i]) && (sockets[i]->address.sa_family == AF_UNIX ||
        sockets[i]->address.sa_family == AF_LOCAL) &&
        !strcmp((char *)&sockets[i]->address.sa_data, addr->sa_data)) {
            return sockets[i];
        }
    }

    return NULL;
}

/* socketLock(): acquires the socket descriptor spinlock
 * params: none
 * returns: nothing
 */

void socketLock() {
    acquireLockBlocking(&lock);
}

/* socketRelease(): frees the socket descriptor spinlock
 * params: none
 * returns: nothing
 */

void socketRelease() {
    releaseLock(&lock);
}

/* socketRegister(): registers an open socket
 * params: sock - socket descriptor structure
 * returns: positive socket index on success, negative error code on fail
 */

int socketRegister(SocketDescriptor *sock) {
    if(socketCount >= MAX_SOCKETS) return -ENFILE;

    for(int i = 0; i < MAX_SOCKETS; i++) {
        if(!sockets[i]) {
            sockets[i] = sock;
            socketCount++;
            return i;
        }
    }

    return -ENFILE;
}

/* socketUnregister(): unregisters an open socket
 * params: index - global index
 * returns: pointer to the socket on success, NULL on fail
 */

SocketDescriptor *socketUnregister(int index) {
    if(!socketCount) return NULL;

    for(int i = 0; i < MAX_SOCKETS; i++) {
        SocketDescriptor *sd = sockets[i];
        if(sd && (sd->globalIndex == index)) {
            sockets[i] = NULL;
            socketCount--;
            return sd;
        }
    }

    return NULL;
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
    else p = getProcess(getKernelPID());

    if(!p) return -ESRCH;
    if(p->iodCount == MAX_IO_DESCRIPTORS) return -EMFILE;
    if(socketCount >= MAX_SOCKETS) return -ENFILE;

    acquireLockBlocking(&lock);

    IODescriptor *iod = NULL;       // open I/O descriptor
    int sd = openIO(p, (void **) &iod);
    if(sd < 0 || !iod) return sd;

    iod->type = IO_SOCKET;
    iod->data = calloc(1, sizeof(SocketDescriptor));
    iod->flags = type >> 8;
    if(!iod->data) {
        closeIO(p, iod);
        releaseLock(&lock);
        return -ENOMEM;
    }

    // set up the socket family for now
    SocketDescriptor *sock = (SocketDescriptor *)iod->data;
    sock->refCount = 1;
    sock->process = p;
    sock->address.sa_family = domain;
    sock->type = type & 0xFF;
    sock->protocol = protocol;
    sock->globalIndex = socketRegister(sock);

    if(sock->globalIndex < 0) {
        closeIO(p, iod);
        releaseLock(&lock);
        return -ENFILE;
    }

    releaseLock(&lock);
    return sd;
}

/* bind(): assigns a local address to a socket
 * params: t - calling thread, NULL for kernel threads
 * params: sd - socket descriptor
 * params: addr - socket address structure
 * params: len - length of socket address struct
 * returns: zero on success, negative error code on fail
 */

int bind(Thread *t, int sd, const struct sockaddr *addr, socklen_t len) {
    if(sd < 0 || sd >= MAX_IO_DESCRIPTORS) return -EBADF;
    Process *p;
    if(t) p = getProcess(t->pid);
    else p = getProcess(getKernelPID());
    if(!p) return -ESRCH;

    // input verification
    if(len > sizeof(struct sockaddr)) len = sizeof(struct sockaddr);
    if(!p->io[sd].valid || p->io[sd].type != IO_SOCKET) return -ENOTSOCK;

    acquireLockBlocking(&lock);

    SocketDescriptor *sock = (SocketDescriptor *) p->io[sd].data;
    if(!sock) {
        releaseLock(&lock);
        return -ENOTSOCK;
    }

    if(addr->sa_family != sock->address.sa_family) {
        releaseLock(&lock);
        return -EAFNOSUPPORT;
    }

    // finally
    memcpy(&sock->address, addr, len);
    sock->addressLength = len;
    releaseLock(&lock);
    return 0;
}

/* closeSocket(): closes a socket
 * params: t - calling thread
 * params: sd - socket descriptor
 * returns: 1 on success, negative error code on fail
 */

int closeSocket(Thread *t, int sd) {
    if(sd < 0 || sd >= MAX_IO_DESCRIPTORS) return -EBADF;
    Process *p;
    if(t) p = getProcess(t->pid);
    else p = getProcess(getKernelPID());
    if(!p) return -ESRCH;

    acquireLockBlocking(&lock);
    SocketDescriptor *sock = (SocketDescriptor *) p->io[sd].data;
    if(!sock) {
        releaseLock(&lock);
        return -EBADF;
    }

    sock->refCount--;
    if(sock->refCount) {
        releaseLock(&lock);
        return 1;
    }

    if(sock->peer) {
        // disconnect the socket from its peer
        // TODO: for future TCP sockets, terminate the connection here
        sock->peer->peer = NULL;
        sock->peer = NULL;
    }

    // and delete the socket
    socketUnregister(sock->globalIndex);
    free(sock);
    closeIO(p, &p->io[sd]);
    releaseLock(&lock);
    return 1;
}