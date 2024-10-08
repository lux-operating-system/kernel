/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <platform/lock.h>
#include <kernel/boot.h>
#include <kernel/font.h>
#include <kernel/tty.h>
#include <kernel/memory.h>
#include <kernel/logger.h>

KTTY ktty;
static lock_t lock = LOCK_INITIAL;

const uint32_t ttyColors[] = {
    0x1F1F1F,       // black
    0x990000,       // red
    0x00A600,       // green
    0x999900,       // yellow
    0x0000B2,       // blue
    0xB200B2,       // magenta
    0x00A6B2,       // cyan
    0xBFBFBF,       // white
    0x666666,       // gray
    0xE60000,       // bright red
    0x00D900,       // bright green
    0xE6E600,       // bright yellow
    0x0000FF,       // bright blue
    0xE600E6,       // bright magenta
    0x00E6E6,       // bright cyan
    0xE6E6E6        // bright white
};

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
    ktty.bg = ttyColors[0];
    ktty.fg = ttyColors[7];
    ktty.bpp = boot->bpp;
    ktty.bytesPerPixel = (ktty.bpp + 7) / 8;

    // clear the screen
    uint32_t *fb = ktty.fb;

    for(int i = 0; i < ktty.h; i++) {
        for(int j = 0; j < ktty.w; j++) {
            fb[j] = ktty.bg;
        }

        fb = (uint32_t *)((uintptr_t)fb + ktty.pitch);
    }

    // show signs of life
    /*KDEBUG("kernel tty initialized\n");
    KDEBUG("screen resolution is %dx%dx%d bpp\n", ktty.w, ktty.h, ktty.bpp);
    KDEBUG("terminal resolution is %dx%d\n", ktty.wc, ktty.hc);*/
}

/* ttyRemapFramebuffer(): this is called after paging is initialized */

void ttyRemapFramebuffer() {
    ktty.fb = (uint32_t *)vmmMMIO((uintptr_t)ktty.fb, true);
}

/* ttyCreateBackbuffer(): creates the back buffer after the mm is initialized
 */

void ttyCreateBackbuffer() {
    ktty.fbhw = malloc(ktty.pitch*ktty.h);
    if(!ktty.fbhw) {
        KERROR("unable to allocate memory for back buffer\n");
        for(;;);
    }

    uint32_t *temp = ktty.fb;
    ktty.fb = ktty.fbhw;
    ktty.fbhw = temp;

    memcpy(ktty.fb, ktty.fbhw, ktty.pitch*ktty.h);
}

/* ttyRedraw(): redraws the back buffer 
 * params: line - line to redraw, -1 for entire screen
 * returns: nothing
 */

void ttyRedraw(int line) {
    if(!ktty.fbhw) return;
    if(line < 0) {
        memcpy(ktty.fbhw, ktty.fb, ktty.pitch*ktty.h);
        return;
    }

    size_t size = ktty.pitch * FONT_HEIGHT;
    uintptr_t offset = line * size;
    memcpy((void *)((uintptr_t)ktty.fbhw + offset), (void *)((uintptr_t)ktty.fb + offset), size);
}

/* ttyCheckBoundaries(): checks for the cursor position and scrolls
 */

void ttyCheckBoundaries() {
    if(ktty.posx >= ktty.wc) {
        ktty.posx = 0;
        ktty.posy++;
    }

    if(ktty.posy >= ktty.hc) {
        // scroll up by one line
        uint32_t *secondLine = (uint32_t *)((uintptr_t)ktty.fb + (FONT_HEIGHT*ktty.pitch));
        size_t size = (ktty.hc - 1) * FONT_HEIGHT * ktty.pitch;
        memcpy(ktty.fb, secondLine, size);

        // clear the scrolled line, which is also pointed to by size
        uint32_t *lastLine = (uint32_t *)((uintptr_t)ktty.fb + size);
        for(int i = 0; i < FONT_HEIGHT; i++) {
            for(int j = 0; j < ktty.w; j++) {
                lastLine[j] = ktty.bg;
            }

            lastLine = (uint32_t *)((uintptr_t)lastLine + ktty.pitch);
        }

        ktty.posx = 0;
        ktty.posy = ktty.hc - 1;

        ttyRedraw(-1);
    }
}

/* ttyParseEscape(): parses an escape sequence
 */

void ttyParseEscape() {
    int bufferIndex = 0;
    int command;
    char buffer[32];
    memset(buffer, 0, 32);

    if(ktty.escape[0] != '[') return;   // only the color sequences are implemented

    for(int i = 1; i <= ktty.escapeIndex; i++) {
        // check if we found a numerical value
        if(ktty.escape[i] >= '0' && ktty.escape[i] <= '9') {
            buffer[bufferIndex] = ktty.escape[i];
            bufferIndex++;
        } else {
            // found a non-numerical value, so now process the value we do have
            command = atoi(buffer);
            if(command >= 30 && command <= 37) {
                // set foreground color (dark)
                ktty.fg = ttyColors[command - 30];
            } else if(command >= 40 && command <= 47) {
                // set background color (dark)
                ktty.bg = ttyColors[command - 40];
            } else if(command >= 90 && command <= 97) {
                // set foreground color (light)
                ktty.fg = ttyColors[command - 82];
            } else if(command >= 100 && command <= 107) {
                // set background color (light)
                ktty.bg = ttyColors[command - 92];
            }

            bufferIndex = 0;
            memset(buffer, 0, 32);
        }
    }
}

/* ttyPutc(): puts a character on screen at cursor position
 * params: c - character
 * returns: nothing
 */

void ttyPutc(char c) {
    acquireLockBlocking(&lock);

    // handle special characters
    if(c == '\n') {             // new line
        ktty.posx = 0;
        ktty.posy++;
        ttyRedraw(ktty.posy-1);
        ttyCheckBoundaries();
        releaseLock(&lock);
        return;
    } else if(c == '\r') {      // carriage return
        ktty.posx = 0;
        releaseLock(&lock);
        return;
    } else if(c == '\e') {      // escape
        ktty.escapeIndex = 0;
        memset(ktty.escape, 0, 256);
        ktty.escaping = true;
        releaseLock(&lock);
        return;
    }

    if(ktty.escaping) {
        ktty.escape[ktty.escapeIndex] = c;
        ktty.escapeIndex++;

        // the only sequences implemented are the color sequences
        if(c == 'm') {
            ktty.escaping = false;
            ttyParseEscape();
        }

        releaseLock(&lock);
        return;
    }

    if(c < FONT_MIN_GLYPH || c > FONT_MAX_GLYPH) {
        releaseLock(&lock);
        return;
    }

    // get pixel offset
    uint16_t x = ktty.posx * FONT_WIDTH;
    uint16_t y = ktty.posy * FONT_HEIGHT;
    uint32_t *fb = (uint32_t *)((uintptr_t)ktty.fb + (y * ktty.pitch) + (x * ktty.bytesPerPixel));

    // and font data
    const uint8_t *fontPtr = &font[(c - FONT_MIN_GLYPH)*FONT_HEIGHT];

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
    ttyCheckBoundaries();
    releaseLock(&lock);
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

/* getTtyStatus(): returns the status of the framebuffer and emulated terminal
 * params: b - buffer to store the status
 * returns: nothing
 */

void getTtyStatus(KTTY *b) {
    memcpy(b, &ktty, sizeof(KTTY));
}
