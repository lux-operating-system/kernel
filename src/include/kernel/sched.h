/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

#pragma once

#include <stdint.h>
#include <stddef.h>

// this gives us a maximum of over a million running processes
#define MAX_PID                 0xFFFFF

#define THREAD_QUEUED           0
#define THREAD_RUNNING          1
#define THREAD_BLOCKED          2   // waiting for I/O

typedef uint32_t pid_t;
typedef uint32_t uid_t;
typedef uint32_t gid_t;

typedef struct Thread {
    int status;
    pid_t pid, tid;         // pid == tid for the main thread
    uint64_t time;

    struct Thread *next;
    void *context;          // platform-specific (page tables, registers, etc)
} Thread;

typedef struct Process {
    pid_t pid, parent;
    uid_t user;
    gid_t group;

    char *env;              // environmental variables
    char *command;          // command line with arguments

    size_t threadCount;
    size_t childrenCount;

    Thread **threads;
    struct Process **children;  // array of pointers of size childrenCount
    struct Process *next;
} Process;

void schedInit();
void schedCycle();
