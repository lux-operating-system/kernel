/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Platform-Specific Code for x86_64
 */

#include <stdint.h>

// wrappers for instructions that don't have an equivalent in C
uint32_t read_cr0();
void write_cr0(uint32_t);
uint64_t read_cr2();
uint64_t read_cr3();
void write_cr3(uint64_t);
uint32_t read_cr4();
void write_cr4(uint32_t);
void lgdt(void *);
void lidt(void *);
