/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <platform/mmap.h>
#include <kernel/sched.h>
#include <kernel/socket.h>
#include <kernel/syscalls.h>
#include <kernel/logger.h>

/* This is the dispatcher for system calls, many of which need a wrapper for
 * their behavior. This ensures the exposed functionality is always as close
 * as possible to the Unix specification */

/* syscallVerifyPointer(): ensure user programs don't violate memory permissions
 * params: req - syscall request
 * params: base - base pointer
 * params: len - length of the structure at the point
 * returns: true if safe, false if unsafe, user program terminated as well
 */

bool syscallVerifyPointer(SyscallRequest *req, uintptr_t base, uintptr_t len) {
    uintptr_t end = base + len;
    if(base < USER_BASE_ADDRESS || end > USER_LIMIT_ADDRESS) {
        KWARN("killing tid %d for memory access violation at 0x%X (%d)\n", req->thread->tid, base, len);
        terminateThread(req->thread, -1, false);
        return false;
    }

    return true;
}

/* Group 1: Scheduler */

void syscallDispatchExit(SyscallRequest *req) {
    exit(req->thread, req->params[0]);
}

void syscallDispatchFork(SyscallRequest *req) {
    req->ret = fork(req->thread);
    req->unblock = true;
}

void syscallDispatchYield(SyscallRequest *req) {
    req->ret = yield(req->thread);
    req->unblock = true;
}

void syscallDispatchGetPID(SyscallRequest *req) {
    req->ret = req->thread->pid;
    req->unblock = true;
}

void syscallDispatchGetTID(SyscallRequest *req) {
    req->ret = req->thread->tid;
    req->unblock = true;
}

void syscallDispatchGetUID(SyscallRequest *req) {
    Process *p = getProcess(req->thread->pid);
    if(!p) {
        KWARN("process is a null pointer in getuid() for tid %d\n", req->thread->tid);
        req->ret = (int64_t)-1;
    } else {
        req->ret = p->user;
    }

    req->unblock = true;
}

void syscallDispatchGetGID(SyscallRequest *req) {
    Process *p = getProcess(req->thread->pid);
    if(!p) {
        KWARN("process is a null pointer in getgid() for tid %d\n", req->thread->tid);
        req->ret = (int64_t)-1;
    } else {
        req->ret = p->group;
    }

    req->unblock = true;
}

void syscallDispatchMSleep(SyscallRequest *req) {
    req->ret = msleep(req->thread, req->params[0]);
}

/* TODO: Group 2: File System */

/* Group 3: Interprocess Communication */

void syscallDispatchSocket(SyscallRequest *req) {
    req->ret = socket(req->thread, req->params[0], req->params[1], req->params[2]);
    req->unblock = true;
}

void syscallDispatchConnect(SyscallRequest *req) {
    if(syscallVerifyPointer(req, req->params[1], req->params[2])) {
        req->ret = connect(req->thread, req->params[0], (const struct sockaddr *)req->params[1], req->params[2]);
        req->unblock = true;
    }
}

void syscallDispatchBind(SyscallRequest *req) {
    if(syscallVerifyPointer(req, req->params[1], req->params[2])) {
        req->ret = bind(req->thread, req->params[0], (const struct sockaddr *)req->params[1], req->params[2]);
        req->unblock = true;
    }
}

void syscallDispatchListen(SyscallRequest *req) {
    req->ret = listen(req->thread, req->params[0], req->params[1]);
    req->unblock = true;
}

void syscallDispatchAccept(SyscallRequest *req) {
    int status;
    if(!req->params[1]) {
        status = accept(req->thread, req->params[0], NULL, NULL);
    } else {
        if(syscallVerifyPointer(req, req->params[1], sizeof(const struct sockaddr)) &&
        syscallVerifyPointer(req, req->params[2], sizeof(socklen_t))) {
            status = accept(req->thread, req->params[0], (struct sockaddr *)req->params[1], (socklen_t *)req->params[2]);
        }
    }

    if(status == -EWOULDBLOCK || status == -EAGAIN) {
        // return without unblocking if necessary
        Process *p = getProcess(req->thread->pid);
        if(!(p->io[req->params[0]].flags & O_NONBLOCK)) {
            // block by putting the syscall back in the queue
            req->unblock = false;
            req->busy = false;
            req->queued = true;
            req->next = NULL;
            syscallEnqueue(req);
            return;
        }
    }

    req->ret = status;
    req->unblock = true;
}

void syscallDispatchRecv(SyscallRequest *req) {
    if(syscallVerifyPointer(req, req->params[1], req->params[2])) {
        ssize_t status = recv(req->thread, req->params[0], (void *)req->params[1], req->params[2], req->params[3]);

        // block the thread if necessary
        if(status == -EWOULDBLOCK || status == -EAGAIN) {
            // return without unblocking if necessary
            Process *p = getProcess(req->thread->pid);
            if(!(p->io[req->params[0]].flags & O_NONBLOCK)) {
                // block by putting the syscall back in the queue
                req->unblock = false;
                req->busy = false;
                req->queued = true;
                req->next = NULL;
                syscallEnqueue(req);
                return;
            }
        }

        req->ret = status;
        req->unblock = true;
    }
}

void syscallDispatchSend(SyscallRequest *req) {
    if(syscallVerifyPointer(req, req->params[1], req->params[2])) {
        ssize_t status = send(req->thread, req->params[0], (const void *)req->params[1], req->params[2], req->params[3]);

        // block the thread if necessary
        if(status == -EWOULDBLOCK || status == -EAGAIN) {
            // return without unblocking if necessary
            Process *p = getProcess(req->thread->pid);
            if(!(p->io[req->params[0]].flags & O_NONBLOCK)) {
                // block by putting the syscall back in the queue
                req->unblock = false;
                req->busy = false;
                req->queued = true;
                req->next = NULL;
                syscallEnqueue(req);
                return;
            }
        }

        req->ret = status;
        req->unblock = true;
    }
}

void (*syscallDispatchTable[])(SyscallRequest *) = {
    /* group 1: scheduler functions */
    syscallDispatchExit,        // 0 - exit()
    syscallDispatchFork,        // 1 - fork()
    syscallDispatchYield,       // 2 - yield()
    NULL,                       // 3 - waitpid()
    NULL,                       // 4 - execve()
    NULL,                       // 5 - execrdv()
    syscallDispatchGetPID,      // 6 - getpid()
    syscallDispatchGetTID,      // 7 - gettid()
    syscallDispatchGetUID,      // 8 - getuid()
    syscallDispatchGetGID,      // 9 - getgid()
    NULL,                       // 10 - setuid()
    NULL,                       // 11 - setgid()
    syscallDispatchMSleep,      // 12 - msleep()

    /* group 2: file system manipulation */
    NULL,                       // 13 - open()
    NULL,                       // 14 - close()
    NULL,                       // 15 - read()
    NULL,                       // 16 - write()
    NULL,                       // 17 - stat()
    NULL,                       // 18 - lseek()
    NULL,                       // 19 - chown()
    NULL,                       // 20 - chmod()
    NULL,                       // 21 - link()
    NULL,                       // 22 - unlink()
    NULL,                       // 23 - mknod()
    NULL,                       // 24 - mkdir()
    NULL,                       // 25 - rmdir()
    NULL,                       // 26 - utime()
    NULL,                       // 27 - chroot()
    NULL,                       // 28 - mount()
    NULL,                       // 29 - umount()
    NULL,                       // 30 - fnctl()

    /* group 3: interprocess communication */
    syscallDispatchSocket,      // 31 - socket()
    syscallDispatchConnect,     // 32 - connect()
    syscallDispatchBind,        // 33 - bind()
    syscallDispatchListen,      // 34 - listen()
    syscallDispatchAccept,      // 35 - accept()
    syscallDispatchRecv,        // 36 - recv()
    syscallDispatchSend,        // 37 - send()
};
