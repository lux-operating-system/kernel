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

/* openIO(): opens an I/O descriptor in a process
 * params: p - process to open descriptor in
 * params: iod - destination to store pointer to I/O descriptor structure
 * returns: I/O descriptor, negative error code on fail
 */

int openIO(void *pv, void **iodv) {
    Process *p = (Process *)pv;
    IODescriptor *iod = (IODescriptor *)iodv;

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
