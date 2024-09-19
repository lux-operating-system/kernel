/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

#pragma once

#include <stdint.h>
#include <kernel/sched.h>
#include <kernel/irq.h>

/* routines that must be implemented and constants tha must be defined by any
 * platform-specific code, abstracting the difference between different platforms;
 * the goal is to keep the core kernel portable across CPU architectures */

#define PLATFORM_TIMER_FREQUENCY            1000        // Hz, this is for the scheduler

#define PLATFORM_PAGE_PRESENT               0x0001      // present in main memory
#define PLATFORM_PAGE_SWAP                  0x0002      // present in storage device
#define PLATFORM_PAGE_USER                  0x0004      // user-kernel toggle
#define PLATFORM_PAGE_EXEC                  0x0008
#define PLATFORM_PAGE_WRITE                 0x0010
#define PLATFORM_PAGE_NO_CACHE              0x0020
#define PLATFORM_PAGE_ERROR                 0x8000      // all bits invalid if this bit is set

int platformCPUSetup();         // very early setup for one CPU
int platformPagingSetup();      // paging setup for virtual memory management
uintptr_t platformGetPage(int *, uintptr_t);     // get physical address and flags of a page
uintptr_t platformMapPage(uintptr_t, uintptr_t, int);    // map a physical address to a virtual address
int platformUnmapPage(uintptr_t);               // and vice versa

int platformRegisterCPU(void *);    // registers a CPU, relevant to multiprocessor systems
int platformCountCPU();
void *platformGetCPU(int);          // find CPU structure from index
int platformWhichCPU();             // index of current CPU
uint64_t platformUptime();          // total uptime of boot CPU
void platformAcknowledgeIRQ(void *);    // must be called at the end of interrupt handlers
void platformInitialSeed();
uint64_t platformRand();
void platformSeed(uint64_t);
void platformSaveContext(void *, void *);   // save scheduler context
void platformLoadContext(void *);           // load scheduler context
void platformSwitchContext(Thread *);       // perform a context switch
void platformHalt();                        // halt the CPU until the next context switch
void *platformGetPagingRoot();
void *platformCloneKernelSpace();           // clone kernel thread page tables
void *platformCloneUserSpace(uintptr_t);    // clone user thread page tables
pid_t platformGetPid();
pid_t platformGetTid();
int platformUseContext(void *);     // use the paging context of a thread without switching context
SyscallRequest *platformCreateSyscallContext(Thread *); // create syscall context from register state
void *platformCloneContext(void *, const void *);   // for fork()
void platformSetContextStatus(void *, uint64_t);    // store syscall return value in the context
int platformIoperm(Thread *, uintptr_t, uintptr_t, int);    // request I/O port access
int platformGetMaxIRQ();        // maximum interrupt implemented by hardware
int platformConfigureIRQ(Thread *, int, IRQHandler *);  // configure an IRQ pin
