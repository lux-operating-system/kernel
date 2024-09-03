/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

#include <stdint.h>
#include <kernel/sched.h>
#include <kernel/syscalls.h>

/* This is the dispatcher for system calls, many of which need a wrapper for
 * their behavior. This ensures the exposed functionality is always as close
 * as possible to the Unix specification */

void syscallDispatchFork(SyscallRequest *req) {
    req->ret = fork(req->thread);
}

void syscallDispatchYield(SyscallRequest *req) {
    req->ret = yield(req->thread);
}

void (*syscallDispatchTable[])(SyscallRequest *) = {
    // group 1: scheduler functions
    NULL,                       // 0 - exit()
    syscallDispatchFork,        // 1 - fork()
    syscallDispatchYield,       // 2 - yield()
    NULL,                       // 3 - execve()
    NULL,                       // 4 - execrd()
    //syscallDispatchGetPID,      // 5 - getpid()
    //syscallDispatchGetUID,      // 6 - getuid()
    //syscallDispatchGetGID,      // 7 - getgid()
};
