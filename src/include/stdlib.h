/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Kernel-Level Partial Implementation of the C Library
 */

#pragma once

#define OCTAL       8
#define DECIMAL     10
#define HEX         16

char *itoa(int, char *, int);
int atoi(const char *);
char *ltoa(long, char *, int);
long atol(const char *);
