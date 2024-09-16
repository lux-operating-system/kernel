/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

/* Helper functions for syscalls that depend on user space servers */

#include <errno.h>
#include <kernel/servers.h>
#include <kernel/socket.h>

extern int lumenSocket;

/* requestServer(): sends a request message to lumen
 * params: t - requesting thread
 * params: msg - pointer to message
 * returns: 0 on success
 */

int requestServer(Thread *t, void *msg) {
    SyscallHeader *hdr = (SyscallHeader *)msg;
    hdr->header.requester = t->tid;

    ssize_t s = send(NULL, lumenSocket, hdr, hdr->header.length, 0);
    if(s == hdr->header.length) return 0;
    else if(s >= 0) return -ENOBUFS;
    else return s;
}
