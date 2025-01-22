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
#include <limits.h>
#include <sys/types.h>
#include <platform/lock.h>
#include <kernel/syscalls.h>
#include <kernel/io.h>

typedef struct Process Process;
typedef struct Thread Thread;

#include <kernel/signal.h>

// still not sure of how to decide on this value so it'll probably change
#define SCHED_TIME_SLICE        1       // ms

#define MAX_PID                 99999

#define THREAD_QUEUED           0
#define THREAD_RUNNING          1
#define THREAD_BLOCKED          2       // waiting for I/O
#define THREAD_ZOMBIE           3
#define THREAD_SLEEP            4

#define PRIORITY_NORMAL         1
#define PRIORITY_HIGH           2
#define PRIORITY_HIGHEST        3

// exit status
#define EXIT_NORMAL             0x100
#define EXIT_SIGNALED           0x200

// waitpid() flags
#define WCONTINUED              0x01
#define WNOHANG                 0x02
#define WUNTRACED               0x04

typedef struct SignalQueue {
    struct SignalQueue *next;
    int signum;
    struct Thread *sender;
} SignalQueue;

struct Thread {
    int status, cpu, priority;
    pid_t pid, tid;         // pid == tid for the main thread
    uint64_t time;          // timeslice OR sleep time if sleeping thread
    lock_t lock;

    bool normalExit;        // true when the thread ends by exit() and is not forcefully killed
    bool clean;             // true when the exit status has been read by waitpid()
    bool handlingSignal;    // true inside a signal handler

    void *signals;
    sigset_t signalMask;
    SignalQueue *signalQueue;
    uintptr_t signalTrampoline;
    uintptr_t siginfo;
    uintptr_t signalUserContext;

    SyscallRequest syscall; // for when the thread is blocked
    int exitStatus;         // for zombie threads

    int pages;              // memory pages used

    struct Thread *next;
    void *context;          // platform-specific (page tables, registers, etc)
    void *signalContext;

    uintptr_t highest;
};

struct Process {
    pid_t pid, parent, pgrp;
    uid_t user;
    gid_t group;
    mode_t umask;           // file creation mask

    bool orphan;            // true when the parent process exits or is killed
    bool zombie;            // true when all threads are zombies

    char command[ARG_MAX*32];   // command line with arguments
    char name[MAX_PATH];        // file name

    struct IODescriptor io[MAX_IO_DESCRIPTORS];
    int iodCount;

    char cwd[MAX_PATH];

    int pages;              // memory pages used

    size_t threadCount;
    size_t childrenCount;

    Thread **threads;
    struct Process **children;  // array of pointers of size childrenCount
    struct Process *next;
};

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
void schedStatus();
bool schedBusy();

pid_t kthreadCreate(void *(*)(void *), void *);
pid_t processCreate();
int threadUseContext(pid_t);
void setLocalSched(bool);

pid_t execveMemory(const void *, const char **argv, const char **envp);
pid_t getLumenPID();
void setLumenPID(pid_t);
void setKernelPID(pid_t);
pid_t getKernelPID();
int schedException(pid_t, pid_t);
void terminateThread(Thread *, int, bool);
void schedSleepTimer();
Thread *getKernelThread();
void threadCleanup(Thread *);

// these functions are exposed as system calls, but some will need to take
// the thread as an argument from the system call handler - the actual user
// application does not need to be aware of which thread is running for
// Unix compatibility
int yield(Thread *);
pid_t fork(Thread *);
void exit(Thread *, int);
int execve(Thread *, uint16_t, const char *, const char **, const char **);
int execveHandle(void *);
int execrdv(Thread *, const char *, const char **);
unsigned long msleep(Thread *, unsigned long);
pid_t waitpid(Thread *, pid_t, int *, int);
