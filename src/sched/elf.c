/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

#include <string.h>
#include <kernel/logger.h>
#include <kernel/elf.h>
#include <kernel/memory.h>
#include <platform/mmap.h>

/*
 * loadELF(): loads the sections of an ELF file
 * params: binary - pointer to the ELF header
 * params: highest - pointer to where to store the binary's highest address
 * returns: absolute address of the entry point, zero on fail
 */

uint64_t loadELF(const void *binary, uint64_t *highest) {
    uint8_t *ptr = (uint8_t *)binary;
    ELFFileHeader *header = (ELFFileHeader *)ptr;

    uint64_t addr = 0;

    if(header->magic[0] != 0x7F || header->magic[1] != 'E' ||
    header->magic[2] != 'L' || header->magic[3] != 'F') {
        KWARN("ELF file does not contain valid signature\n");
        return 0;
    }

    if(header->isaWidth != ELF_ISA_WIDTH_64) {
        KWARN("ELF file is not 64-bit\n");
        return 0;
    }

    if(header->isa != ELF_ARCHITECTURE) {
        KWARN("ELF file is for an unsupported architecture\n");
        return 0;
    }

    if(header->type != ELF_TYPE_EXEC) {
        return 0;
    }

    if(!header->headerEntryCount) {
        return 0;
    }

    // load the segments
    ELFProgramHeader *prhdr = (ELFProgramHeader *)(ptr + header->headerTable);
    for(int i = 0; i < header->headerEntryCount; i++) {
        if(prhdr->segmentType == ELF_SEGMENT_TYPE_NULL) {
            /* ignore null entries and do nothing*/
        } else if(prhdr->segmentType == ELF_SEGMENT_TYPE_LOAD) {
            // verify the segments are within the user space region of memory
            if((prhdr->virtualAddress < USER_BASE_ADDRESS) ||
            ((prhdr->virtualAddress+prhdr->memorySize) > USER_LIMIT_ADDRESS)) {
                return 0;
            }

            uint64_t end = prhdr->virtualAddress + prhdr->memorySize;
            if(end > addr) {
                addr = end;     // maintain the highest address
            }

            // allocate memory with permissions that match the program section
            size_t pages = (prhdr->memorySize + PAGE_SIZE - 1) / PAGE_SIZE;
            if(prhdr->virtualAddress & (PAGE_SIZE-1)) {
                pages++;
            }

            int flags = VMM_USER | VMM_WRITE;
            if(prhdr->flags & ELF_SEGMENT_FLAGS_EXEC) {
                flags |= VMM_EXEC;
            }

            uintptr_t vp = vmmAllocate(prhdr->virtualAddress, USER_LIMIT_ADDRESS, pages, flags);
            if(!vp) {
                return 0;
            }

            if(vp != (prhdr->virtualAddress & ~(PAGE_SIZE-1))) {
                vmmFree(vp, pages);
            }

            memcpy((void *)prhdr->virtualAddress, (const void *)(binary + prhdr->fileOffset), prhdr->fileSize);

            // adjust the perms by removing the write perms if necessary
            if(!(prhdr->flags & ELF_SEGMENT_FLAGS_WRITE)) {
                flags &= ~VMM_WRITE;
            }

            vmmSetFlags(prhdr->virtualAddress, pages, flags);
        } else {
            /* unimplemented header type */
            KERROR("unimplemented ELF header type %d\n", prhdr->segmentType);
            return 0;
        }

        prhdr = (ELFProgramHeader *)((uintptr_t)prhdr + header->headerEntrySize);
    }

    *highest = addr;
    return header->entryPoint;
}