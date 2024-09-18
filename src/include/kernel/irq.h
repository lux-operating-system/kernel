/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

#pragma once

#include <stdint.h>
#include <kernel/sched.h>

/* Generic platform-independent structure for IRQ handlers */
typedef struct {
    char name[256];     // device/driver name
    int kernel;         // 1 for kernel-level IRQ handler, 0 for user space
    uintptr_t khandler; // entry point for kernel IRQ handlers
    int socket;         // socket for user space driver
} IRQHandler;

typedef struct {
    int pin;            // irq number
    int devices;        // number of devices sharing this IRQ
    IRQHandler *handlers;
} IRQ;

int installIRQ(Thread *, int, IRQHandler *);
