/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Platform-Specific Code for x86_64
 */

#include <stdint.h>
#include <platform/idt.h>
#include <platform/gdt.h>

#pragma once

typedef struct {
    // these are in reverse order because of how the x86 stack works
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rbp;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rbx;
    uint64_t rax;
    uint64_t code;      // ignore
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
} __attribute__((packed)) InterruptRegisters;

typedef struct {
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
} __attribute__((packed)) CPUIDRegisters;

// wrappers for instructions that don't have an equivalent in C
uint64_t readCR0();
void writeCR0(uint64_t);
uint64_t readCR2();
uint64_t readCR3();
void writeCR3(uint64_t);
uint64_t readCR4();
void writeCR4(uint64_t);
void loadGDT(void *);
void loadIDT(void *);
void storeGDT(void *);
void storeIDT(void *);
void loadTSS(uint16_t);
uint16_t storeTSS();
void outb(uint16_t, uint8_t);
void outw(uint16_t, uint16_t);
void outd(uint16_t, uint32_t);
uint8_t inb(uint16_t);
uint16_t inw(uint16_t);
uint32_t ind(uint16_t);
void resetSegments(uint64_t, uint8_t);
uint32_t readCPUID(uint32_t, CPUIDRegisters *);
uint64_t readMSR(uint32_t);
void writeMSR(uint32_t, uint64_t);
void enableIRQs();
void disableIRQs();
void halt();

#define CR0_NOT_WRITE_THROUGH       0x20000000
#define CR0_CACHE_DISABLE           0x40000000  // caching
#define CR4_FSGSBASE                0x00010000  // enable fs/gs segmentation

// other x86_64-specific routines
extern GDTEntry gdt[];
extern IDTEntry idt[];
extern GDTR gdtr;
extern IDTR idtr;
void installInterrupt(uint64_t, uint16_t, int, int, int);

#define PRIVILEGE_KERNEL        0
#define PRIVILEGE_USER          3

#define INTERRUPT_TYPE_INT      0x0E
#define INTERRUPT_TYPE_TRAP     0x0F

#define MSR_EFER                0xC0000080
#define MSR_FS_BASE             0xC0000100
#define MSR_GS_BASE             0xC0000101
#define MSR_GS_BASE_KERNEL      0xC0000102  // for swapgs
#define MSR_STAR                0xC0000081  // CS and SS
#define MSR_LSTAR               0xC0000082  // syscall 64-bit entry point
#define MSR_CSTAR               0xC0000083  // syscall 32-bit entry point, probably never gonna use this
#define MSR_SFMASK              0xC0000084  // 32-bit EFLAGS mask, upper 32 bits reserved

#define MSR_EFER_SYSCALL        0x00000001  // syscall/sysret instructions
#define MSR_EFER_NX_ENABLE      0x00000800  // PAE/NX
#define MSR_EFER_FFXSR          0x00004000  // fast FXSAVE/FXRSTOR

// paging
#define PT_PAGE_PRESENT         0x0001
#define PT_PAGE_RW              0x0002
#define PT_PAGE_USER            0x0004
#define PT_PAGE_WRITE_THROUGH   0x0008
#define PT_PAGE_NO_CACHE        0x0010
#define PT_PAGE_SIZE_EXTENSION  0x0080
#define PT_PAGE_NXE             ((uint64_t)0x8000000000000000)   // SET to disable execution privilege

// page fault status code
#define PF_PRESENT              0x01
#define PF_WRITE                0x02
#define PF_USER                 0x04
#define PF_RESERVED_WRITE       0x08
#define PF_FETCH                0x10
// there are more causes of page faults, but these are the only implemented ones

