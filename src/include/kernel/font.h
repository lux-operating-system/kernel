/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

/*
 * This is the Modern DOS font from https://github.com/susam/pcface/, which is
 * used and redistributed under the Creative Commons License.
 */

#pragma once

#include <stdint.h>

#define FONT_WIDTH              8
#define FONT_HEIGHT             16
#define FONT_MIN_GLYPH          32      // ' '
#define FONT_MAX_GLYPH          126     // '~'

extern const uint8_t font[];
