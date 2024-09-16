/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

/* Kernel-Server Communication */

#include <string.h>
#include <kernel/servers.h>
#include <kernel/sched.h>
#include <kernel/syscalls.h>
#include <kernel/logger.h>

void handleSyscallResponse(const SyscallHeader *hdr) {
    SyscallRequest *req = getSyscall(hdr->header.requester);
    if(!req) {
        KWARN("received response for syscall 0x%X pid %d but no such request exists\n", hdr->header.command, hdr->header.requester);
        return;
    }

    if(req->requestID != hdr->id) {
        KWARN("received response for syscall 0x%X (kernel syscall %d) pid %d but IDs mismatch; terminating thread\n", hdr->header.command, req->function, hdr->header.requester);
        schedLock();
        terminateThread(req->thread, -1, false);
        schedRelease();
        return;
    }

    // unblock the thread
    schedLock();
    req->ret = hdr->header.status;
    req->external = false;
    req->unblock = true;
    req->thread->time = schedTimeslice(req->thread, req->thread->priority);
    req->thread->status = THREAD_QUEUED;

    // some syscalls will require writing to the thread's memory
    switch(hdr->header.command) {
    case COMMAND_STAT:
        if(hdr->header.status) break;

        StatCommand *statcmd = (StatCommand *) hdr;
        threadUseContext(req->thread->tid);
        memcpy((void *)req->params[1], &statcmd->buffer, sizeof(struct stat));
        break;
    }

    schedRelease();
}
