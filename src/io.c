/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

/* Abstractions for file systems and sockets */

#include <errno.h>
#include <stdlib.h>
#include <platform/platform.h>
#include <kernel/sched.h>
#include <kernel/io.h>
#include <kernel/socket.h>
#include <kernel/file.h>
#include <kernel/logger.h>

/* openIO(): opens an I/O descriptor in a process
 * params: p - process to open descriptor in
 * params: iod - destination to store pointer to I/O descriptor structure
 * returns: I/O descriptor, negative error code on fail
 */

int openIO(void *pv, void **iodv) {
    Process *p = (Process *)pv;
    IODescriptor **iod = (IODescriptor **)iodv;

    if(p->iodCount >= MAX_IO_DESCRIPTORS) return -ESRCH;

    /* randomly allocate descriptors instead of sequential numbering */
    int desc;
    do {
        desc = rand() % MAX_IO_DESCRIPTORS;
    } while(p->io[desc].valid);

    p->io[desc].valid = true;
    p->io[desc].type = IO_WAITING;
    p->io[desc].data = NULL;

    p->iodCount++;

    *iod = &p->io[desc];
    return desc;
}

/* closeIO(): closes an I/O descriptor in a process
 * params: pv - process to close descriptor in
 * params: iodv - descriptor to close
 * returns: nothing
 */

void closeIO(void *pv, void *iodv) {
    Process *p = (Process *)pv;
    IODescriptor *iod = (IODescriptor *) iodv;

    if(iod->valid) {
        iod->valid = false;
        if(iod->data) free(iod->data);
        iod->data = NULL;

        p->iodCount--;
    }
}

/* read(): reads from an I/O descriptor and relays the call to a file or socket
 * params: t - calling thread, NULL for kernel threads
 * params: id - syscall ID
 * params: fd - file or socket descriptor
 * params: buffer - buffer to read into
 * params: count - number of bytes to read
 * returns: number of bytes actually read, negative error code on fail
 */

ssize_t read(Thread *t, uint64_t id, int fd, void *buffer, size_t count) {
    Process *p;
    if(t) p = getProcess(t->pid);
    else p = getProcess(getKernelPID());
    if(!p) return -ESRCH;

    if(!p->io[fd].valid || !p->io[fd].data) return -EBADF;

    // relay the call to the appropriate file or socket handler
    if(p->io[fd].type == IO_SOCKET) return recv(t, fd, buffer, count, 0);
    else if(p->io[fd].type == IO_FILE) return readFile(t, id, &p->io[fd], buffer, count);
    else return -EBADF;
}

/* write(): writes to an I/O descriptor and relays the call to a file or socket
 * params: t - calling thread, NULL for kernel threads
 * params: id - syscall ID
 * params: fd - file or socket descriptor
 * params: buffer - buffer to write from
 * params: count - number of bytes to write
 * returns: number of bytes actually written, negative error code on fail
 */

ssize_t write(Thread *t, uint64_t id, int fd, const void *buffer, size_t count) {
    Process *p;
    if(t) p = getProcess(t->pid);
    else p = getProcess(getKernelPID());
    if(!p) return -ESRCH;

    if(!p->io[fd].valid || !p->io[fd].data) return -EBADF;

    // relay the call to the appropriate file or socket handler
    if(p->io[fd].type == IO_SOCKET) return send(t, fd, buffer, count, 0);
    else if(p->io[fd].type == IO_FILE) return writeFile(t, id, &p->io[fd], buffer, count);
    else return -EBADF;
}

/* close(): closes an I/O descriptor
 * params: t - calling thread, NULL for kernel threads
 * params: fd - file or socket descriptor
 * returns: zero on success, negative error code on fail
 */

int close(Thread *t, int fd) {
    Process *p;
    if(t) p = getProcess(t->pid);
    else p = getProcess(getKernelPID());
    if(!p) return -ESRCH;

    if(!p->io[fd].valid || !p->io[fd].data) return -EBADF;

    if(p->io[fd].type == IO_SOCKET) return closeSocket(t, fd);
    else if(p->io[fd].type == IO_FILE) return closeFile(t, fd);
    else return -EBADF;
}

/* ioperm(): sets the I/O permissions for the current thread
 * params: t - calling thread
 * params: from - base I/O port
 * params: count - number of I/O ports to change permissions
 * params: enable - 0 to disable access, 1 to enable
 * returns: zero on success, negative error code on fail
 */

int ioperm(struct Thread *t, uintptr_t from, uintptr_t count, int enable) {
    // not all platforms implement I/O ports and this is really relevant to x86
    // on platforms that don't implement I/O ports, simply return -EIO

    // check permissions
    Process *p = getProcess(t->pid);
    if(!p) return -ESRCH;
    if(p->user) return -EPERM;  // only root is allowed to access I/O ports

    schedLock();

    int status = platformIoperm(t, from, count, enable);
    if(!status)
        KDEBUG("thread %d %s access to I/O ports 0x%04X-0x%04X\n", t->tid, enable ? "was granted" : "revoked", from, from+count-1);
    else
        KWARN("thread %d was denied access to I/O ports 0x%04X-0x%04X\n", t->tid, from, from+count-1);

    schedRelease();
    return status;
}

/* ioctl(): manipulate parameters of character special device files
 * params: t - calling thread
 * params: fd - file descriptor
 * params: op - opcode, these are device-specific
 * params: optional pointer (char *) argument is opcode-specific
 * returns: zero on success, negative error code on fail
 */

int ioctl(struct Thread *t, int fd, unsigned long op, ...) {
    return -EIO;    // stub
}