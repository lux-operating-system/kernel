/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

#include <stdint.h>
#include <kernel/sched.h>
#include <kernel/syscalls.h>
#include <kernel/logger.h>

/* This is the dispatcher for system calls, many of which need a wrapper for
 * their behavior. This ensures the exposed functionality is always as close
 * as possible to the Unix specification */

void syscallDispatchFork(SyscallRequest *req) {
    req->ret = fork(req->thread);
}

void syscallDispatchYield(SyscallRequest *req) {
    req->ret = yield(req->thread);
}

void syscallDispatchGetPID(SyscallRequest *req) {
    req->ret = req->thread->pid;
}

void syscallDispatchGetTID(SyscallRequest *req) {
    req->ret = req->thread->tid;
}

void syscallDispatchGetUID(SyscallRequest *req) {
    Process *p = getProcess(req->thread->pid);
    if(!p) {
        KWARN("process is a null pointer in getuid() for tid %d\n", req->thread->tid);
        req->ret = (int64_t)-1;
    } else {
        req->ret = p->user;
    }
}

void syscallDispatchGetGID(SyscallRequest *req) {
    Process *p = getProcess(req->thread->pid);
    if(!p) {
        KWARN("process is a null pointer in getgid() for tid %d\n", req->thread->tid);
        req->ret = (int64_t)-1;
    } else {
        req->ret = p->group;
    }
}

void (*syscallDispatchTable[])(SyscallRequest *) = {
    // group 1: scheduler functions
    NULL,                       // 0 - exit()
    syscallDispatchFork,        // 1 - fork()
    syscallDispatchYield,       // 2 - yield()
    NULL,                       // 3 - execve()
    NULL,                       // 4 - execrd()
    syscallDispatchGetPID,      // 5 - getpid()
    syscallDispatchGetTID,      // 6 - gettid()
    syscallDispatchGetUID,      // 7 - getuid()
    syscallDispatchGetGID,      // 8 - getgid()
};
