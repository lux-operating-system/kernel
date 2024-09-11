/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

/* Helper functions for syscalls that depend on user space servers */

#include <kernel/servers.h>
#include <kernel/socket.h>

extern int lumenSocket;

/* requestServer(): sends a request message to lumen
 * params: t - requesting thread
 * params: msg - pointer to message
 * returns: nothing
 */

void requestServer(Thread *t, void *msg) {
    SyscallHeader *hdr = (SyscallHeader *)msg;
    hdr->header.requester = t->tid;

    send(NULL, lumenSocket, hdr, hdr->header.length, 0);
}
