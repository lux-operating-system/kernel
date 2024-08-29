/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <kernel/boot.h>

#define MAX_MODULES         16

void modulesInit(KernelBootInfo *);
int moduleCount();
size_t moduleQuery(const char *);
void *moduleLoad(void *, const char *);

struct USTARMetadata {
    char name[100];
    char mode[8];
    char owner[8];
    char group[8];
    char size[12];
    char modified[12];
    char checksum[8];
    char type;
    char link[100];
    char magic[6];      // "ustar"
    char version[2];    // '00'
    char ownerName[32];
    char groupName[32];
    char deviceMajorNumber[8];
    char deviceMinorNumber[8];
    char namePrefix[155];
} __attribute__((packed));

void ramdiskInit(KernelBootInfo *);
struct USTARMetadata *ramdiskFind(const char *);
int64_t ramdiskFileSize(const char *);
