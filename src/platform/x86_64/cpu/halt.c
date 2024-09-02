/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Platform-Specific Code for x86_64
 */

#include <platform/x86_64.h>
#include <platform/platform.h>

void platformHalt() {
    enableIRQs();
    halt();
}
