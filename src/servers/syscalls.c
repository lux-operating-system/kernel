/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

/* Kernel-Server Communication */

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
    schedRelease();
}
