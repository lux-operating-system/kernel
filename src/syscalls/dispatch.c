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
#include <platform/platform.h>
#include <kernel/sched.h>
#include <kernel/socket.h>
#include <kernel/syscalls.h>
#include <kernel/logger.h>
#include <kernel/memory.h>
#include <kernel/file.h>
#include <kernel/irq.h>
#include <kernel/dirent.h>
#include <kernel/signal.h>
#include <sys/types.h>
#include <sys/stat.h>

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

/* syscallID(): generates a random non-zero syscall ID
 * params: none
 * returns: non-zero random number
 */

static uint16_t syscallID() {
    uint16_t r = 0;
    while(!r) r = platformRand() & 0xFFFF;
    return r;
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
    req->ret = 0;
    req->unblock = true;
}

void syscallDispatchWaitPID(SyscallRequest *req) {
    if(syscallVerifyPointer(req, req->params[1], sizeof(int))) {
        pid_t status = waitpid(req->thread, req->params[0], (int *) req->params[1], req->params[2]);
        
        // block if necessary
        if((!status) && (!(req->params[2] & WNOHANG))) {
            req->unblock = false;
            req->busy = false;
            req->queued = true;
            req->next = NULL;
            req->retry = true;
            syscallEnqueue(req);
        } else {
            req->ret = status;
            req->unblock = true;
        }
    }
}

void syscallDispatchExecve(SyscallRequest *req) {
    if(syscallVerifyPointer(req, req->params[0], MAX_FILE_PATH) &&
    syscallVerifyPointer(req, req->params[1], ARG_MAX*sizeof(uintptr_t)) &&
    syscallVerifyPointer(req, req->params[2], ARG_MAX*sizeof(uintptr_t))) {
        req->requestID = syscallID();
        int status = execve(req->thread, req->requestID, (const char *) req->params[0],
            (const char **) req->params[1], (const char **) req->params[2]);
        if(status) {
            // error code
            req->external = false;
            req->ret = status;
            req->unblock = true;
        } else {
            // block until completion
            req->external = true;
            req->unblock = false;
        }
    }
}

void syscallDispatchExecrdv(SyscallRequest *req) {
    req->ret = execrdv(req->thread, (const char *) req->params[0], (const char **) req->params[1]);
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

/* Group 2: File System */

void syscallDispatchOpen(SyscallRequest *req) {
    if(syscallVerifyPointer(req, req->params[0], MAX_FILE_PATH)) {
        req->requestID = syscallID();

        int status = open(req->thread, req->requestID, (const char *)req->params[0], req->params[1], req->params[2]);
        if(status) {
            req->external = false;
            req->ret = status;      // error code
            req->unblock = true;
        } else {
            req->external = true;
            req->unblock = false;
        }
    }
}

void syscallDispatchClose(SyscallRequest *req) {
    req->ret = close(req->thread, req->params[0]);
    req->unblock = true;
}

void syscallDispatchRead(SyscallRequest *req) {
    if(syscallVerifyPointer(req, req->params[1], req->params[2])) {
        uint16_t id;
        if(!req->retry) {
            id = syscallID();
            req->requestID = id;
        } else {
            id = req->requestID;
        }

        ssize_t status = read(req->thread, id, req->params[0], (void *) req->params[1], req->params[2]);
        if(status == -EWOULDBLOCK || status == -EAGAIN) {
            // return without unblocking if necessary for sockets
            Process *p = getProcess(req->thread->pid);
            if(!(p->io[req->params[0]].flags & O_NONBLOCK)) {
                // block by putting the syscall back in the queue
                req->unblock = false;
                req->busy = false;
                req->queued = true;
                req->next = NULL;
                req->retry = true;
                syscallEnqueue(req);
                return;
            }
        } else if(status) {
            req->external = false;
            req->ret = status;      // status or error code
            req->unblock = true;
        } else {
            // block until completion
            req->external = true;
            req->unblock = false;
        }
    }
}

void syscallDispatchWrite(SyscallRequest *req) {
    if(syscallVerifyPointer(req, req->params[1], req->params[2])) {
        uint16_t id;
        if(!req->retry) {
            id = syscallID();
            req->requestID = id;
        } else {
            id = req->requestID;
        }

        ssize_t status = write(req->thread, id, req->params[0], (void *) req->params[1], req->params[2]);
        if(status == -EWOULDBLOCK || status == -EAGAIN) {
            // return without unblocking if necessary for sockets
            Process *p = getProcess(req->thread->pid);
            if(!(p->io[req->params[0]].flags & O_NONBLOCK)) {
                // block by putting the syscall back in the queue
                req->unblock = false;
                req->busy = false;
                req->queued = true;
                req->next = NULL;
                req->retry = true;
                syscallEnqueue(req);
                return;
            }
        } else if(status) {
            req->external = false;
            req->ret = status;      // status or error code
            req->unblock = true;
        } else {
            // block until completion
            req->external = true;
            req->unblock = false;
        }
    }
}

void syscallDispatchStat(SyscallRequest *req) {
    if(syscallVerifyPointer(req, req->params[0], MAX_FILE_PATH) && syscallVerifyPointer(req, req->params[1], sizeof(struct stat))) {
        req->requestID = syscallID();

        int status = stat(req->thread, req->requestID, (const char *)req->params[0], (struct stat *)req->params[1]);
        if(status) {
            req->external = false;
            req->ret = status;      // error code
            req->unblock = true;
        } else {
            // block until completion
            req->external = true;
            req->unblock = false;
        }
    }
}

void syscallDispatchFStat(SyscallRequest *req) {
    if(syscallVerifyPointer(req, req->params[1], sizeof(struct stat))) {
        req->requestID = syscallID();

        int status = fstat(req->thread, req->requestID, req->params[0], (struct stat *)req->params[1]);
        if(status) {
            req->external = false;
            req->ret = status;      // error code
            req->unblock = true;
        } else {
            // block until completion
            req->external = true;
            req->unblock = false;
        }
    }
}

void syscallDispatchLSeek(SyscallRequest *req) {
    req->ret = lseek(req->thread, req->params[0], req->params[1], req->params[2]);
    req->unblock = true;
}

void syscallDispatchChdir(SyscallRequest *req) {
    if(syscallVerifyPointer(req, req->params[0], MAX_FILE_PATH)) {
        req->requestID = syscallID();

        int status = chdir(req->thread, req->requestID, (const char *) req->params[0]);
        if(status) {
            req->external = false;
            req->ret = status;      // error code
            req->unblock = true;
        } else {
            // block until completion
            req->external = true;
            req->unblock = false;
        }
    }
}

void syscallDispatchGetCWD(SyscallRequest *req) {
    req->ret = (uint64_t) getcwd(req->thread, (char *) req->params[0], req->params[1]);
    req->unblock = true;
}

void syscallDispatchMount(SyscallRequest *req) {
    if(syscallVerifyPointer(req, req->params[0], MAX_FILE_PATH) &&
    syscallVerifyPointer(req, req->params[1], MAX_FILE_PATH) &&
    syscallVerifyPointer(req, req->params[2], 32)) {
        req->requestID = syscallID();

        int status = mount(req->thread, req->requestID, (const char *)req->params[0], (const char *)req->params[1], (const char *)req->params[2], req->params[3]);
        if(status) {
            req->external = false;
            req->ret = status;      // error code
            req->unblock = true;
        } else {
            // block until completion
            req->external = true;
            req->unblock = false;
        }
    }
}

void syscallDispatchFcntl(SyscallRequest *req) {
    req->ret = fcntl(req->thread, req->params[0], req->params[1], req->params[2]);
    req->unblock = true;
}

void syscallDispatchOpendir(SyscallRequest *req) {
    if(syscallVerifyPointer(req, req->params[0], MAX_FILE_PATH)) {
        req->requestID = syscallID();

        int status = opendir(req->thread, req->requestID, (const char *)req->params[0]);
        if(status) {
            req->external = false;
            req->ret = status;
            req->unblock = true;
        } else {
            req->external = true;
            req->unblock = false;
        }
    }
}

void syscallDispatchClosedir(SyscallRequest *req) {
    req->ret = closedir(req->thread, (DIR *) req->params[0]);
    req->unblock = true;
}

void syscallDispatchReaddir(SyscallRequest *req) {
    if(syscallVerifyPointer(req, req->params[1], sizeof(struct dirent)) &&
        syscallVerifyPointer(req, req->params[2], sizeof(struct dirent *))) {
        req->requestID = syscallID();

        int status = readdir_r(req->thread, req->requestID, (DIR *) req->params[0], (struct dirent *) req->params[1], (struct dirent **) req->params[2]);
        if(status) {
            req->external = false;
            req->ret = status;
            req->unblock = true;
        } else {
            req->external = true;
            req->unblock = false;
        }
    }
}

void syscallDispatchSeekdir(SyscallRequest *req) {
    seekdir(req->thread, (DIR *) req->params[0], req->params[1]);
    req->unblock = true;
}

void syscallDispatchTelldir(SyscallRequest *req) {
    req->ret = telldir(req->thread, (DIR *) req->params[0]);
    req->unblock = true;
}

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
    int status = -EWOULDBLOCK;
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

void syscallDispatchKill(SyscallRequest *req) {
    req->ret = kill(req->thread, req->params[0], req->params[1]);
    req->unblock = true;
}

void syscallDispatchSigAction(SyscallRequest *req) {
    if((!req->params[1] || syscallVerifyPointer(req, req->params[1], sizeof(struct sigaction))) &&
    (!req->params[2] || syscallVerifyPointer(req, req->params[2], sizeof(struct sigaction)))) {
        req->ret = sigaction(req->thread, req->params[0], (const struct sigaction *) req->params[1], (struct sigaction *) req->params[2]);
        req->unblock = true;
    }
}

void syscallDispatchSigreturn(SyscallRequest *req) {
    sigreturn(req->thread);
    req->unblock = true;
}

/* Group 4: Memory Management */

void syscallDispatchSBrk(SyscallRequest *req) {
    req->ret = (uint64_t) sbrk(req->thread, (intptr_t) req->params[0]);
    req->unblock = true;
}

void syscallDispatchMmap(SyscallRequest *req) {
    if(syscallVerifyPointer(req, req->params[0], sizeof(struct MmapSyscallParams))) {
        req->requestID = syscallID();
        struct MmapSyscallParams *p = (struct MmapSyscallParams *) req->params[0];
        intptr_t status = (intptr_t) mmap(req->thread, req->requestID, p->addr, p->len,
            p->prot, p->flags, p->fd, p->off);
        if(status < 0) {
            req->ret = status;
            req->unblock = true;
        } else {
            // block until completion
            req->external = true;
            req->unblock = false;
        }
    }
}

/* Group 5: Driver I/O Functions */

void syscallDispatchIoperm(SyscallRequest *req) {
    req->ret = ioperm(req->thread, req->params[0], req->params[1], req->params[2]);
    req->unblock = true;
}

void syscallDispatchIRQ(SyscallRequest *req) {
    if(syscallVerifyPointer(req, req->params[1], sizeof(IRQHandler))) {
        req->ret = installIRQ(req->thread, req->params[0], (IRQHandler *) req->params[1]);
        req->unblock = true;
    }
}

void syscallDispatchIoctl(SyscallRequest *req) {
    unsigned long op = req->params[1];
    req->requestID = syscallID();

    if(op & IOCTL_OUT_PARAM) {
        if(syscallVerifyPointer(req, req->params[2], sizeof(unsigned long))) {
            ioctl(req->thread, req->requestID, req->params[0], req->params[1], (unsigned long *)req->params[2]);
        }
    } else {
        ioctl(req->thread, req->requestID, req->params[0], req->params[1], req->params[2]);
    }

    req->external = true;
    req->unblock = false;
}

void syscallDispatchMMIO(SyscallRequest *req) {
    req->ret = mmio(req->thread, req->params[0], req->params[1], req->params[2]);
    req->unblock = true;
}

void syscallDispatchPContig(SyscallRequest *req) {
    req->ret = pcontig(req->thread, req->params[0], req->params[1], req->params[2]);
    req->unblock = true;
}

void syscallDispatchVToP(SyscallRequest *req) {
    req->ret = vtop(req->thread, req->params[0]);
    req->unblock = true;
}

void (*syscallDispatchTable[])(SyscallRequest *) = {
    /* group 1: scheduler functions */
    syscallDispatchExit,        // 0 - exit()
    syscallDispatchFork,        // 1 - fork()
    syscallDispatchYield,       // 2 - yield()
    syscallDispatchWaitPID,     // 3 - waitpid()
    syscallDispatchExecve,      // 4 - execve()
    syscallDispatchExecrdv,     // 5 - execrdv()
    syscallDispatchGetPID,      // 6 - getpid()
    syscallDispatchGetTID,      // 7 - gettid()
    syscallDispatchGetUID,      // 8 - getuid()
    syscallDispatchGetGID,      // 9 - getgid()
    NULL,                       // 10 - setuid()
    NULL,                       // 11 - setgid()
    syscallDispatchMSleep,      // 12 - msleep()
    NULL,                       // 13 - times()

    /* group 2: file system manipulation */
    syscallDispatchOpen,        // 14 - open()
    syscallDispatchClose,       // 15 - close()
    syscallDispatchRead,        // 16 - read()
    syscallDispatchWrite,       // 17 - write()
    syscallDispatchStat,        // 18 - stat()
    syscallDispatchFStat,       // 19 - fstat()
    syscallDispatchLSeek,       // 20 - lseek()
    NULL,                       // 21 - chown()
    NULL,                       // 22 - chmod()
    NULL,                       // 23 - link()
    NULL,                       // 24 - unlink()
    NULL,                       // 25 - mknod()
    NULL,                       // 26 - mkdir()
    NULL,                       // 27 - rmdir()
    NULL,                       // 28 - utime()
    NULL,                       // 29 - chroot()
    syscallDispatchChdir,       // 30 - chdir()
    syscallDispatchGetCWD,      // 31 - getcwd()
    syscallDispatchMount,       // 32 - mount()
    NULL,                       // 33 - umount()
    syscallDispatchFcntl,       // 34 - fcntl()
    syscallDispatchOpendir,     // 35 - opendir()
    syscallDispatchClosedir,    // 36 - closedir()
    syscallDispatchReaddir,     // 37 - readdir_r()
    syscallDispatchSeekdir,     // 38 - seekdir()
    syscallDispatchTelldir,     // 39 - telldir()

    /* group 3: interprocess communication */
    syscallDispatchSocket,      // 40 - socket()
    syscallDispatchConnect,     // 41 - connect()
    syscallDispatchBind,        // 42 - bind()
    syscallDispatchListen,      // 43 - listen()
    syscallDispatchAccept,      // 44 - accept()
    syscallDispatchRecv,        // 45 - recv()
    syscallDispatchSend,        // 46 - send()
    syscallDispatchKill,        // 47 - kill()
    syscallDispatchSigAction,   // 48 - sigaction()
    syscallDispatchSigreturn,   // 49 - sigreturn()

    /* group 4: memory management */
    syscallDispatchSBrk,        // 50 - sbrk()
    syscallDispatchMmap,        // 51 - mmap()
    NULL,                       // 52 - munmap()

    /* group 5: driver I/O functions */
    syscallDispatchIoperm,      // 53 - ioperm()
    syscallDispatchIRQ,         // 54 - irq()
    syscallDispatchIoctl,       // 55 - ioctl()
    syscallDispatchMMIO,        // 56 - mmio()
    syscallDispatchPContig,     // 57 - pcontig()
    syscallDispatchVToP,        // 58 - vtop()
};
