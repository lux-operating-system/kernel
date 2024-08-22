/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Platform-Specific Code for x86_64
 */

#include <platform/x86_64.h>
#include <platform/exception.h>

void installExceptions() {

}

void exception(uint64_t number, uint64_t code) {
    while (1);
}