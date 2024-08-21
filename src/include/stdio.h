/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Kernel-Level Partial Implementation of the C Library
 */

#pragma once

#include <stdarg.h>

int putchar(int);
int puts(const char *);
int printf(const char *, ...);
int vprintf(const char *, va_list);
