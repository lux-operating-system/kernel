/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

/* Kernel-Server Communication */

#include <stdlib.h>
#include <string.h>
#include <platform/platform.h>
#include <kernel/logger.h>
#include <kernel/socket.h>
#include <kernel/servers.h>

static int kernelSocket = 0, lumenSocket = 0;
static int *connections;           // connected socket descriptors
static struct sockaddr *connaddr;  // connected socket addresses
static socklen_t *connlen;         // length of connected socket addresses
static void *buffer;
static int connectionCount = 0;

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
    buffer = malloc(SERVER_MAX_SIZE);

    if(!connections || !connaddr || !connlen || !buffer) {
        KERROR("failed to allocate memory for incoming connections\n");
        while(1) platformHalt();
    }

    KDEBUG("kernel is listening on socket %d: %s\n", kernelSocket, addr.sun_path);
}

/* serverIdle(): handles incoming kernel connections when idle
 * params: none
 * returns: nothing
 */

void serverIdle() {
    // check for incoming connections
    int sd = accept(NULL, kernelSocket, &connaddr[connectionCount], &connlen[connectionCount]);
    if(sd > 0) {
        KDEBUG("kernel accepted connection from %s\n", sd, connaddr[connectionCount].sa_data);
        connections[connectionCount] = sd;
        connectionCount++;
    }

    // check if any of the incoming connections sent anything
    if(!connectionCount) return;
    for(int i = 0; i < connectionCount; i++) {
        sd = connections[i];
        if(recv(NULL, sd, buffer, SERVER_MAX_SIZE, 0) > 0) {
            // TODO: handle
        }
    }
}