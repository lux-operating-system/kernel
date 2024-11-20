/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Kernel-Level Partial Implementation of the C Library
 */

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <kernel/memory.h>
#include <platform/platform.h>
#include <platform/lock.h>

static lock_t lock = LOCK_INITIAL;

struct mallocHeader {
    uint64_t byteSize;
    uint64_t pageSize;
};

char *itoa(int n, char *buffer, int radix) {
    return ltoa((long)n, buffer, radix);
}

int atoi(const char *s) {
    return (long)atol(s);
}

char *ltoa(long n, char *buffer, int radix) {
    if(!radix || radix > HEX) return NULL;

    if(!n) {
        buffer[0] = '0';
        buffer[1] = 0;
        return buffer;
    }

    int length = 0;

    uint64_t un = (uint64_t)n;

    while(un) {
        // convert digit by digit and then reverse the string
        uint64_t digit = un % radix;

        if(digit >= 10) {
            buffer[length] = 'a' + digit - 10;
        } else {
            buffer[length] = '0' + digit;
        }

        length++;
        un /= radix;
    }

    buffer[length] = 0;   // null terminator

    // now reverse the string
    if(length >= 2) {
        for(int i = 0; i < length/2; i++) {
            char tmp = buffer[i];
            buffer[i] = buffer[length-i-1];
            buffer[length-i-1] = tmp;
        }
    }

    return buffer;
}

long atol(const char *s) {
    long v = 0;
    int len = 0;

    while(s[len] >= '0' && s[len] <= '9') {
        len++;      // didn't use strlen so we can only account for numerical characters
    }

    if(!len) return 0;
    if(len == 1) return (long) s[0] - '0';

    long multiplier = 1;
    for(int i = 1; i < len; i++) {
        multiplier *= 10;
    }

    for(int i = 0; i < len; i++) {
        long digit = s[i] - '0';
        v += (digit * multiplier);
        multiplier /= 10;
    }

    return v;
}

void *malloc(size_t size) {
    if(!size) return NULL;
    size_t pageSize = (size + sizeof(struct mallocHeader) + PAGE_SIZE - 1) / PAGE_SIZE;

    acquireLockBlocking(&lock);

    // allocate memory with kernel permissions, write permissions, and no execute
    // there's probably never a scenario where it's a good idea to execute code
    // in a block of memory allocated by malloc() or its derivatives
    uintptr_t ptr = vmmAllocate(KERNEL_HEAP_BASE, KERNEL_HEAP_LIMIT, pageSize, VMM_WRITE);
    if(!ptr) {
        releaseLock(&lock);
        return NULL;
    }

    struct mallocHeader *header = (struct mallocHeader *)ptr;
    header->byteSize = size;
    header->pageSize = pageSize;

    // allocate a guard page as well
    uintptr_t guard = ptr + (pageSize*PAGE_SIZE);
    platformMapPage(guard, 0, 0);

    releaseLock(&lock);

    return (void *)((uintptr_t)ptr + sizeof(struct mallocHeader));
}

void *umalloc(size_t size) {
    /* this is the exact same as malloc() but allocates in the user space to
     * be used for signal structures */
    if(!size) return NULL;
    size_t pageSize = (size + sizeof(struct mallocHeader) + PAGE_SIZE - 1) / PAGE_SIZE;

    acquireLockBlocking(&lock);

    uintptr_t ptr = vmmAllocate(USER_HEAP_BASE, USER_HEAP_LIMIT, pageSize, VMM_WRITE | VMM_USER);
    if(!ptr) {
        releaseLock(&lock);
        return NULL;
    }

    struct mallocHeader *header = (struct mallocHeader *)ptr;
    header->byteSize = size;
    header->pageSize = pageSize;

    // allocate a guard page as well
    uintptr_t guard = ptr + (pageSize*PAGE_SIZE);
    platformMapPage(guard, 0, 0);

    releaseLock(&lock);

    return (void *)((uintptr_t)ptr + sizeof(struct mallocHeader));
}

void *uxmalloc(size_t size) {
    /* again exactly the same umalloc() but with execute permissions, this will
     * be used for installing platform-specific signal trampoline code */
    if(!size) return NULL;
    size_t pageSize = (size + sizeof(struct mallocHeader) + PAGE_SIZE - 1) / PAGE_SIZE;

    acquireLockBlocking(&lock);

    uintptr_t ptr = vmmAllocate(USER_HEAP_BASE, USER_HEAP_LIMIT, pageSize, VMM_WRITE | VMM_USER | VMM_EXEC);
    if(!ptr) {
        releaseLock(&lock);
        return NULL;
    }

    struct mallocHeader *header = (struct mallocHeader *)ptr;
    header->byteSize = size;
    header->pageSize = pageSize;

    // allocate a guard page as well
    uintptr_t guard = ptr + (pageSize*PAGE_SIZE);
    platformMapPage(guard, 0, 0);

    releaseLock(&lock);

    return (void *)((uintptr_t)ptr + sizeof(struct mallocHeader));
}

void *calloc(size_t num, size_t size) {
    void *ptr = malloc(num * size);
    if(!ptr) return NULL;
    memset(ptr, 0, num * size);
    return ptr;
}

void *ucalloc(size_t num, size_t size) {
    void *ptr = umalloc(num * size);
    if(!ptr) return NULL;
    memset(ptr, 0, num * size);
    return ptr;
}

void *realloc(void *ptr, size_t newSize) {
    if(!newSize) return NULL;
    if(!ptr) return malloc(newSize);

    void *newPtr = malloc(newSize);
    if(!newPtr) return NULL;

    uintptr_t oldBase = (uintptr_t)ptr;
    oldBase &= ~(PAGE_SIZE-1);
    struct mallocHeader *header = (struct mallocHeader *)oldBase;
    size_t oldSize = header->byteSize;

    if(oldSize > newSize) {
        // we're shrinking the memory, copy the new size only
        memcpy(newPtr, ptr, newSize);
    } else {
        memcpy(newPtr, ptr, oldSize);
    }

    free(ptr);
    return newPtr;
}

void *urealloc(void *ptr, size_t newSize) {
    if(!newSize) return NULL;
    if(!ptr) return umalloc(newSize);

    void *newPtr = umalloc(newSize);
    if(!newPtr) return NULL;

    uintptr_t oldBase = (uintptr_t)ptr;
    oldBase &= ~(PAGE_SIZE-1);
    struct mallocHeader *header = (struct mallocHeader *)oldBase;
    size_t oldSize = header->byteSize;

    if(oldSize > newSize) {
        // we're shrinking the memory, copy the new size only
        memcpy(newPtr, ptr, newSize);
    } else {
        memcpy(newPtr, ptr, oldSize);
    }

    free(ptr);
    return newPtr;
}

void free(void *ptr) {
    if(!ptr) return;
    uintptr_t base = (uintptr_t)ptr;
    base &= ~(PAGE_SIZE-1);
    struct mallocHeader *header = (struct mallocHeader *)base;
    vmmFree(base, header->pageSize);
}

int rand() {
    uint64_t r = platformRand();
    int v = r ^ (r >> 32);
    if(v < 0) v *= -1;
    return v % (RAND_MAX + 1);
}

void srand(unsigned int s) {
    platformSeed((uint64_t)s);
}
