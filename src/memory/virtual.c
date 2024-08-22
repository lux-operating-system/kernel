/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

#include <platform/platform.h>
#include <kernel/memory.h>
#include <kernel/logger.h>

void vmmInit() {
    if(platformPagingSetup()) {
        KERROR("failed to create paging structures; cannot initialize virtual memory manager\n");
        while(1);
    }


}