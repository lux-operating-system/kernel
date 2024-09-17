/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

/* Kernel-Server Communication */

#pragma once

#include <stdint.h>
#include <kernel/sched.h>
#include <kernel/file.h>
#include <sys/stat.h>

#define SERVER_MAX_CONNECTIONS  512
#define SERVER_MAX_SIZE         0x80000             // max msg size is 512 KiB
#define SERVER_KERNEL_PATH      "lux:///kernel"     // not a real file, special path
#define SERVER_LUMEN_PATH       "lux:///lumen"      // likewise not a real file

/* these commands are requested by lumen and the servers and fulfilled by the kernel */
#define COMMAND_LOG             0x0000  // output to kernel log
#define COMMAND_SYSINFO         0x0001  // get uptime, mem usage, etc
#define COMMAND_RAND            0x0002  // random number
#define COMMAND_IO              0x0003  // request I/O access
#define COMMAND_PROCESS_IO      0x0004  // get process I/O privileges
#define COMMAND_PROCESS_LIST    0x0005  // get list of processes/threads
#define COMMAND_PROCESS_STATUS  0x0006  // get status of process/thread
#define COMMAND_FRAMEBUFFER     0x0007  // request frame buffer access

#define MAX_GENERAL_COMMAND     0x0007

/* these commands are requested by the kernel for lumen to fulfill syscall requests */
#define COMMAND_STAT            0x8000
#define COMMAND_FLUSH           0x8001
#define COMMAND_MOUNT           0x8002
#define COMMAND_UMOUNT          0x8003
#define COMMAND_OPEN            0x8004
#define COMMAND_READ            0x8005
#define COMMAND_WRITE           0x8006

#define MAX_SYSCALL_COMMAND     0x8006

typedef struct {
    uint16_t command;
    uint16_t length;
    uint8_t response;       // 0 for requests, 1 for response
    uint8_t reserved[3];    // for alignment
    uint64_t latency;       // in ms, for responses
    uint64_t status;        // return value for responses
    pid_t requester;
} MessageHeader;

typedef struct {
    MessageHeader header;
    uint64_t id;            // syscall request ID
} SyscallHeader;

/* log command */
typedef struct {
    MessageHeader header;
    int level;
    char server[512];       // null terminated
    char message[];         // variable length, null terminated
} LogCommand;

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
    MessageHeader header;
    uint64_t buffer;        // pointer
    uint16_t w, h, pitch, bpp;
} FramebufferResponse;

/* mount command */
typedef struct {
    SyscallHeader header;
    char source[MAX_FILE_PATH];
    char target[MAX_FILE_PATH];
    char type[32];
    int flags;
} MountCommand;

/* stat() */
typedef struct {
    SyscallHeader header;
    char source[MAX_FILE_PATH];
    char path[MAX_FILE_PATH];
    struct stat buffer;
} StatCommand;

/* open() */
typedef struct {
    SyscallHeader header;
    char path[MAX_FILE_PATH];
    int flags;
    mode_t mode;
    uid_t uid;
    gid_t gid;
} OpenCommand;

/* read() and write() */
typedef struct {
    SyscallHeader header;
    char path[MAX_FILE_PATH];
    int flags;
    mode_t mode;
    uid_t uid;
    gid_t gid;
    size_t position;
    uint8_t data[];
} RWCommand;

void serverInit();
void serverIdle();
void handleGeneralRequest(int, const MessageHeader *, void *);
void handleSyscallResponse(const SyscallHeader *);
int requestServer(Thread *, void *);
