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
    ktty.bpp = boot->bpp;
    ktty.bytesPerPixel = (ktty.bpp + 7) / 8;

    ttyPuts("tty: terminal initialized\n");
}

/* ttyPutc(): puts a character on screen at cursor position
 * params: c - character
 * returns: nothing
 */

void ttyPutc(char c) {
    if(c < FONT_MIN_GLYPH || c > FONT_MAX_GLYPH) return;

    // get pixel offset
    uint16_t x = ktty.posx * FONT_WIDTH;
    uint16_t y = ktty.posy * FONT_HEIGHT;
    uint32_t *fb = (uint32_t *)((uintptr_t)ktty.fb + (y * ktty.pitch) + (x * ktty.bytesPerPixel));

    // and font data
    uint8_t *fontPtr = &font[(c - FONT_MIN_GLYPH)*FONT_HEIGHT];

    for(int i = 0; i < FONT_HEIGHT; i++) {
        uint8_t b = fontPtr[i];

        for(int j = 0; j < FONT_WIDTH; j++) {
            if(b & 0x80) {
                fb[j] = ktty.fg;
            } else {
                fb[j] = ktty.bg;
            }

            b <<= 1;
        }

        fb = (uint32_t *)((uintptr_t)fb + ktty.pitch);
    }

    // and advance the cursor
    ktty.posx++;
    if(ktty.posx >= ktty.wc) {
        ktty.posx = 0;
        ktty.posy++;
        if(ktty.posy >= ktty.hc) {
            // TODO: implement scrolling here
        }
    }
}

/* ttyPuts(): puts a string on screen at cursor position
 * params: s - string
 * returns: nothing 
 */

void ttyPuts(const char *s) {
    while(*s) {
        ttyPutc(*s);
        s++;
    }
}
