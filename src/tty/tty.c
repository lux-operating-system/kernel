/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

#include <string.h>
#include <stdint.h>
#include <kernel/boot.h>
#include <kernel/font.h>
#include <kernel/tty.h>

KTTY ktty;

/* ttyInit(): initializes a minimal kernel terminal for debugging
 * params: boot - kernel boot information structure
 * returns: nothing
 */

void ttyInit(KernelBootInfo *boot) {
    memset(&ktty, 0, sizeof(KTTY));

    ktty.w = boot->width;
    ktty.h = boot->height;
    ktty.wc = ktty.w / FONT_WIDTH;
    ktty.hc = ktty.h / FONT_HEIGHT;
    ktty.pitch = boot->pitch;
    ktty.fb = (uint32_t *)boot->framebuffer;
    ktty.fg = 0x7F7F7F;
}

