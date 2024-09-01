/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>

#define MAX_PID                 80000

#define THREAD_QUEUED           0
#define THREAD_RUNNING          1
#define THREAD_BLOCKED          2   // waiting for I/O

#define PRIORITY_HIGH           0
#define PRIORITY_NORMAL         1
#define PRIORITY_LOW            2

typedef struct Thread {
    int status, cpu, priority;
    pid_t pid, tid;         // pid == tid for the main thread
    uint64_t time;

    struct Thread *next;
    void *context;          // platform-specific (page tables, registers, etc)

    uintptr_t highest;
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

extern int processes, threads;
void schedInit();
void schedLock();
void schedRelease();
uint64_t schedTimer();
pid_t getPid();
pid_t getTid();
void *schedGetState(pid_t);
void schedule();
Process *getProcess(pid_t);
Thread *getThread(pid_t);
uint64_t schedTimeslice(Thread *, int);
void schedAdjustTimeslice();
void setScheduling(bool);

pid_t kthreadCreate(void *(*)(void *), void *);
pid_t processCreate();
int threadUseContext(pid_t);

int execveMemory(const void *, const char **argv, const char **envp);
int execve(const char *, const char **argv, const char **envp);
