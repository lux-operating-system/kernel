/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Platform-Specific Code for x86_64
 */

#include <stdint.h>
#include <platform/idt.h>
#include <platform/gdt.h>

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

// other x86_64-specific routines
extern IDTEntry idt[];
void installInterrupt(uint64_t, uint16_t, int, int, int);

#define PRIVILEGE_KERNEL        0
#define PRIVILEGE_USER          3

#define INTERRUPT_TYPE_INT      0x0E
#define INTERRUPT_TYPE_TRAP     0x0F