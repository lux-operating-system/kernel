/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

#include <stddef.h>
#include <string.h>
#include <kernel/modules.h>
#include <kernel/boot.h>
#include <kernel/logger.h>

/* the ramdisk is really a USTAR archive that'll be used to load early files
 * during early boot before the user space is set up */

static uint8_t *ramdisk;
static uint64_t ramdiskSize;

/* ramdiskInit(): initializes the ramdisk
 * params: boot - boot information structure
 * returns: nothing
 */

void ramdiskInit(KernelBootInfo *boot) {
    if(boot->ramdisk && boot->ramdiskSize) {
        KDEBUG("ramdisk is at 0x%08X\n", boot->ramdisk);
        KDEBUG("ramdisk size is %d KiB\n", boot->ramdiskSize/1024);

        ramdisk = (uint8_t *)boot->ramdisk;
        ramdiskSize = boot->ramdiskSize;
    } else {
        ramdisk = NULL;
        ramdiskSize = 0;
    }
}

/* parseOctal(): parses an octal number written in ASCII characters
 * params: s - string containing octal number
 * returns: integer
 */

static long parseOctal(const char *s) {
    long v = 0;
    int len = 0;

    while(s[len] >= '0' && s[len] <= '7') {
        len++;      // didn't use strlen so we can only account for numerical characters
    }

    if(!len) return 0;

    long multiplier = 1;
    for(int i = 1; i < len; i++) {
        multiplier *= 8;
    }

    for(int i = 0; i < len; i++) {
        long digit = s[i] - '0';
        v += (digit * multiplier);
        multiplier /= 8;
    }

    return v;
}

/* ramdiskFind(): returns pointer to a file on the ramdisk 
 * params: name - file name
 * returns: pointer to the metadata of the file, NULL if non-existent
 */

struct USTARMetadata *ramdiskFind(const char *name) {
    if(!ramdisk) return NULL;

    struct USTARMetadata *ptr = (struct USTARMetadata *)ramdisk;
    size_t offset = 0;

    while(offset < ramdiskSize && !strcmp(ptr->magic, "ustar")) {
        KDEBUG("%s\n", ptr->name);
        if(!strcmp(ptr->name, name)) return ptr;

        size_t size = ((parseOctal(ptr->size) + 511) / 512) + 1;
        size *= 512;

        offset += size;
        ptr = (struct USTARMetadata *)((uintptr_t)ptr + size);
    }

    return NULL;
}

/* ramdiskFileSize(): returns the size of a file on the ramdisk
 * params: name - file name
 * returns: file size in bytes, -1 if non-existent
 */

int64_t ramdiskFileSize(const char *name) {
    struct USTARMetadata *metadata = ramdiskFind(name);
    if(!metadata) return -1;

    else return parseOctal(metadata->size);
}

/* ramdiskRead(): reads a file from the ramdisk
 * params: buffer - buffer to read file into
 * params: name - file name
 * params: n - number of bytes to read
 * returns: number of bytes read
 */

size_t ramdiskRead(void *buffer, const char *name, size_t n) {
    struct USTARMetadata *metadata = ramdiskFind(name);
    if(!metadata) return 0;

    size_t size = parseOctal(metadata->size);
    if(n > size) n = size;

    uint8_t *data = (uint8_t *)(metadata + 512);
    memcpy(buffer, data, n);
    return n;
}
