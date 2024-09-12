/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

/* Kernel-Server Communication */

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <platform/platform.h>
#include <kernel/logger.h>
#include <kernel/socket.h>
#include <kernel/servers.h>

int kernelSocket = 0, lumenSocket = 0;
static int *connections;           // connected socket descriptors
static struct sockaddr *connaddr;  // connected socket addresses
static socklen_t *connlen;         // length of connected socket addresses
static void *in, *out;
static int connectionCount = 0;
static bool lumenConnected = false;
static struct sockaddr_un lumenAddr;

/* serverInit(): initializes the server subsystem
 * params: none
 * returns: nothing
 */

void serverInit() {
    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, SERVER_KERNEL_PATH);     // this is a special path and not a true file

    kernelSocket = socket(NULL, AF_UNIX, SOCK_DGRAM | SOCK_NONBLOCK, 0);    // NEVER block the kernel
    if(kernelSocket < 0) {
        KERROR("failed to open kernel socket: error code %d\n", -1*kernelSocket);
        while(1) platformHalt();
    }

    int status = bind(NULL, kernelSocket, (const struct sockaddr *)&addr, sizeof(struct sockaddr_un));
    if(status) {
        KERROR("failed to bind kernel socket: error code %d\n", -1*status);
        while(1) platformHalt();
    }

    status = listen(NULL, kernelSocket, SERVER_MAX_CONNECTIONS);
    if(status) {
        KERROR("failed to listen to kernel socket: error code %d\n", -1*status);
        while(1) platformHalt();
    }

    connections = calloc(SERVER_MAX_CONNECTIONS, sizeof(int));
    connaddr = calloc(SERVER_MAX_CONNECTIONS, sizeof(struct sockaddr));
    connlen = calloc(SERVER_MAX_CONNECTIONS, sizeof(socklen_t));
    in = malloc(SERVER_MAX_SIZE);
    out = malloc(SERVER_MAX_SIZE);

    if(!connections || !connaddr || !connlen || !in || !out) {
        KERROR("failed to allocate memory for incoming connections\n");
        while(1) platformHalt();
    }

    KDEBUG("kernel is listening on socket %d: %s\n", kernelSocket, addr.sun_path);

    // set up lumen's socket
    lumenSocket = socket(NULL, AF_UNIX, SOCK_DGRAM | SOCK_NONBLOCK, 0);
    if(lumenSocket < 0) {
        KERROR("failed to open socket for lumen: error code %d\n", -1*lumenSocket);
        for(;;) platformHalt();
    }

    // don't do anything else for now, we will try to connect to lumen later
    // after lumen itself is running
    lumenAddr.sun_family = AF_UNIX;
    strcpy(lumenAddr.sun_path, SERVER_LUMEN_PATH);
}

/* serverIdle(): handles incoming kernel connections when idle
 * params: none
 * returns: nothing
 */

void serverIdle() {
    // check for incoming connections
    connlen[connectionCount] = sizeof(struct sockaddr);
    int sd = accept(NULL, kernelSocket, &connaddr[connectionCount], &connlen[connectionCount]);
    if(sd > 0) {
        //KDEBUG("kernel accepted connection from %s\n", connaddr[connectionCount].sa_data);
        connections[connectionCount] = sd;
        connectionCount++;
        if(connectionCount && !lumenConnected) {
            // connect to lumen
            KDEBUG("connected to lumen at socket %d\n", sd);
            lumenConnected = true;
            lumenSocket = sd;
        }
    }

    // check if any of the incoming connections sent anything
    if(!connectionCount) return;

    MessageHeader *h = (MessageHeader *) in;
    for(int i = 0; i < connectionCount; i++) {
        sd = connections[i];
        while(recv(NULL, sd, in, SERVER_MAX_SIZE, 0) > 0) {
            if(h->command <= MAX_GENERAL_COMMAND) handleGeneralRequest(sd, in, out);
            else if(h->command >= 0x8000 && h->command <= MAX_SYSCALL_COMMAND) handleSyscallResponse((SyscallHeader *)h);
            else {
                // TODO
                KWARN("unimplemented message command 0x%02X, dropping...\n", h->command);
            }
        }
    }
}