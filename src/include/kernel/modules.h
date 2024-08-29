/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

#pragma once

#include <stdint.h>
#include <kernel/boot.h>

#define MAX_MODULES         16

void modulesInit(KernelBootInfo *);
void ramdiskInit(KernelBootInfo *);
