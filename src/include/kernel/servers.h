/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

/* Kernel-Server Communication */

#pragma once

#include <stdint.h>

#define SERVER_MAX_CONNECTIONS  512
#define SERVER_MAX_SIZE         0x80000             // max msg size is 512 KiB
#define SERVER_KERNEL_PATH      "lux:///kernel"     // not a real file, special path
#define SERVER_LUMEN_PATH       "lux:///lumen"      // likewise not a real file

/* these commands are requested by lumen and the servers and fulfilled by the kernel */
#define COMMAND_SYSINFO         0x0000  // get uptime, mem usage, etc
#define COMMAND_RAND            0x0001  // random number
#define COMMAND_IO              0x0002  // request I/O access
#define COMMAND_PROCESS_IO      0x0003  // get process I/O privileges
#define COMMAND_PROCESS_LIST    0x0004  // get list of processes/threads
#define COMMAND_PROCESS_STATUS  0x0005  // get status of process/thread
#define COMMAND_FRAMEBUFFER     0x0006  // request frame buffer access

#define MAX_GENERAL_COMMAND     0x0006

/* these commands are requested by the kernel for lumen to fulfill syscall requests */
#define COMMAND_STAT            0x8000
#define COMMAND_FLUSH           0x8001
#define COMMAND_MOUNT           0x8002
#define COMMAND_UMOUNT          0x8003

#define MAX_SYSCALL_COMMAND     0x0003

typedef struct {
    uint16_t command;
    uint16_t length;
    uint8_t response;       // 0 for requests, 1 for response
    uint8_t reserved[3];    // for alignment
    uint64_t latency;       // in ms, for responses
    uint64_t status;        // return value for responses
} MessageHeader;

typedef struct {
    MessageHeader header;
    uint64_t id;            // syscall request ID
} SyscallHeader;

/* sysinfo command */
typedef struct {
    MessageHeader header;
    char kernel[64];        // version string
    uint64_t uptime;
    int maxPid, maxSockets, maxFiles;
    int processes, threads;
    int pageSize;
    int memorySize, memoryUsage;    // in pages
} SysInfoResponse;

/* framebuffer access command */
typedef struct {
    uint64_t buffer;        // pointer
    uint16_t w, h, pitch, bpp;
} FramebufferResponse;

void serverInit();
void serverIdle();
void handleGeneralRequest(int, const MessageHeader *, void *);
void handleSyscallResponse(int, const MessageHeader *);
