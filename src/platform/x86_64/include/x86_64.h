/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Platform-Specific Code for x86_64
 */

#include <stdint.h>

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
