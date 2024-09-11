/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <kernel/boot.h>

typedef struct {
    uint16_t w, h;          // in pixels
    uint16_t wc, hc;        // in characters
    uint8_t bpp, bytesPerPixel;
    uint16_t posx, posy;
    uint32_t fg, bg;
    uint32_t *fb;
    uint32_t pitch;
    char escape[256];
    bool escaping;
    int escapeIndex;
} KTTY;

extern const uint32_t ttyColors[];

void ttyInit(KernelBootInfo *);
void ttyPutc(char);
void ttyPuts(const char *);
void ttyRemapFramebuffer();
void getTtyStatus(KTTY *);
