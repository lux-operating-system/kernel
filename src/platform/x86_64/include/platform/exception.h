/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Platform-Specific Code for x86_64
 */

#include <stdint.h>
#include <platform/x86_64.h>

#pragma once

/* generic exception handler */
void exception(uint64_t, uint64_t, InterruptRegisters *);

void installExceptions();

/* handlers for each exception, these stubs must be written in assembly so we
 * can save register states and perform an IRETQ when the exception is handled */

extern void divideException();          // 0x00 - no error code
extern void debugException();           // 0x01 - error code in DR6
extern void nmiException();             // 0x02 - no error code
extern void breakpointException();      // 0x03 - no error code
extern void overflowException();        // 0x04 - no error code
extern void boundaryException();        // 0x05 - no error code
extern void opcodeException();          // 0x06 - no error code
extern void deviceException();          // 0x07 - no error code
extern void doubleException();          // 0x08 - error code (zero) in stack
extern void tssException();             // 0x0A - error code in stack
extern void segmentException();         // 0x0B - error code in stack
extern void stackException();           // 0x0C - error code in stack
extern void gpException();              // 0x0D - error code in stack
extern void pageException();            // 0x0E - error code in stack; page fault address in CR2
extern void mathException();            // 0x10 - no error code
extern void alignmentException();       // 0x11 - no error code
extern void machineCheckException();    // 0x12 - error code in MSR
extern void xmathException();           // 0x13 - no error code
extern void virtualException();         // 0x14 - no error code
extern void controlException();         // 0x15 - error code in stack

// there are technically two more exception, but those will not be handled
// because we will not implement AMD SVM
