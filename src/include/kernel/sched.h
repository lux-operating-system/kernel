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
#include <kernel/syscalls.h>

// ideal number of context switches per second
// still not sure of how to decide on this value so it'll probably change
#define SCHED_SWITCH_RATE       200

#define MAX_PID                 99999

#define THREAD_QUEUED           0
#define THREAD_RUNNING          1
#define THREAD_BLOCKED          2       // waiting for I/O
#define THREAD_ZOMBIE           3

#define PRIORITY_HIGH           0
#define PRIORITY_NORMAL         1
#define PRIORITY_LOW            2

typedef struct Thread {
    int status, cpu, priority;
    pid_t pid, tid;         // pid == tid for the main thread
    uint64_t time;          //.timeslice

    bool normalExit;        // true when the thread ends by exit() and is not forcefully killed

    SyscallRequest syscall; // for when the thread is blocked
    int exitStatus;         // for zombie threads

    struct Thread *next;
    void *context;          // platform-specific (page tables, registers, etc)

    uintptr_t highest;
} Thread;

typedef struct Process {
    pid_t pid, parent;
    uid_t user;
    gid_t group;

    bool orphan;            // true when the parent process exits or is killed

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
void blockThread(Thread *);
void unblockThread(Thread *);
Process *getProcessQueue();

pid_t kthreadCreate(void *(*)(void *), void *);
pid_t processCreate();
int threadUseContext(pid_t);

int execveMemory(const void *, const char **argv, const char **envp);

// these functions are exposed as system calls, but some will need to take
// the thread as an argument from the system call handler - the actual user
// application does not need to be aware of which thread is running for
// Unix compatibility
int yield(Thread *);
pid_t fork(Thread *);
void exit(Thread *, int);
int execve(Thread *, const char *, const char **argv, const char **envp);
