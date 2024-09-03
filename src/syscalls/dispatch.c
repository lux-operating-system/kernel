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
    req->thread->time = schedTimeslice(req->thread, req->thread->priority);
    req->thread->status = THREAD_QUEUED;
}

void (*syscallDispatchTable[])(SyscallRequest *) = {
    syscallDispatchFork,       // 0
};
