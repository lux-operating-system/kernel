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

// paging
#define PT_PAGE_PRESENT         0x0001
#define PT_PAGE_RW              0x0002
#define PT_PAGE_USER            0x0004
#define PT_PAGE_WRITE_THROUGH   0x0008
#define PT_PAGE_NO_CACHE        0x0010
#define PT_PAGE_SIZE_EXTENSION  0x0080
#define PT_PAGE_NXE             (1 << 63)   // SET to disable execution privilege

// page fault status code
#define PF_PRESENT              0x01
#define PF_WRITE                0x02
#define PF_USER                 0x04
#define PF_RESERVED_WRITE       0x08
#define PF_FETCH                0x10
// there are more causes of page faults, but these are the only implemented ones

