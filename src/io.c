/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

/* Abstractions for file systems and sockets */

#include <errno.h>
#include <stdlib.h>
#include <kernel/sched.h>
#include <kernel/io.h>
#include <kernel/socket.h>
#include <kernel/file.h>

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