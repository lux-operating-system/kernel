/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Kernel-Level Partial Implementation of the C Library
 */

#pragma once

#include <stddef.h>

void *memcpy(void *, const void *, size_t);
size_t strlen(const char *);
char *strcpy(char *, const char *);
void *memset(void *, int, size_t);
int strcmp(const char *, const char *);
int memcmp(const void *, const void *, size_t);
