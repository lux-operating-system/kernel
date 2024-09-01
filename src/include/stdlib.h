/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Kernel-Level Partial Implementation of the C Library
 */

#pragma once

#include <stddef.h>

#define OCTAL       8
#define DECIMAL     10
#define HEX         16

#define RAND_MAX    0x1FFFFFFF      // at least 32767

char *itoa(int, char *, int);
int atoi(const char *);
char *ltoa(long, char *, int);
long atol(const char *);
void *mallocUC(size_t);
void *malloc(size_t);
void *calloc(size_t, size_t);
void *realloc(void *, size_t);
void free(void *);
int rand();
void srand(unsigned int);
