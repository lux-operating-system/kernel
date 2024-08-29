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

void ramdiskInit(KernelBootInfo *);
