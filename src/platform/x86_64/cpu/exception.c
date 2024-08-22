/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Platform-Specific Code for x86_64
 */

#include <platform/x86_64.h>
#include <platform/exception.h>
#include <kernel/logger.h>

static const char *exceptions[] = {
    "Divide error",             // 0x00
    "Debug exception",          // 0x01
    "Non-maskable interrupt",   // 0x02
    "Breakpoint",               // 0x03
    "Overflow",                 // 0x04
    "Boundary range exceeded",  // 0x05
    "Undefined opcode",         // 0x06
    "Device not present",       // 0x07
    "Double fault",             // 0x08
    "Reserved exception",       // 0x09
    "Invalid TSS",              // 0x0A
    "Data segment exception",   // 0x0B
    "Stack segment exception",  // 0x0C
    "General protection fault", // 0x0D
    "Page fault",               // 0x0E
    "Reserved exception",       // 0x0F
    "Math fault",               // 0x10
    "Alignment exception",      // 0x11
    "Machine check fail",       // 0x12
    "Extended math fault",      // 0x13
    "Virtualization fault",     // 0x14
    "Control protection fault", // 0x15
};

void installExceptions() {
    KDEBUG("installing exception handlers...\n");

    installInterrupt((uint64_t)&divideException, GDT_KERNEL_CODE, PRIVILEGE_KERNEL, INTERRUPT_TYPE_TRAP, 0x00);
    installInterrupt((uint64_t)&debugException, GDT_KERNEL_CODE, PRIVILEGE_KERNEL, INTERRUPT_TYPE_TRAP, 0x01);
    installInterrupt((uint64_t)&nmiException, GDT_KERNEL_CODE, PRIVILEGE_KERNEL, INTERRUPT_TYPE_TRAP, 0x02);
    installInterrupt((uint64_t)&breakpointException, GDT_KERNEL_CODE, PRIVILEGE_KERNEL, INTERRUPT_TYPE_TRAP, 0x03);
    installInterrupt((uint64_t)&overflowException, GDT_KERNEL_CODE, PRIVILEGE_KERNEL, INTERRUPT_TYPE_TRAP, 0x04);
    installInterrupt((uint64_t)&boundaryException, GDT_KERNEL_CODE, PRIVILEGE_KERNEL, INTERRUPT_TYPE_TRAP, 0x05);
    installInterrupt((uint64_t)&opcodeException, GDT_KERNEL_CODE, PRIVILEGE_KERNEL, INTERRUPT_TYPE_TRAP, 0x06);
    installInterrupt((uint64_t)&deviceException, GDT_KERNEL_CODE, PRIVILEGE_KERNEL, INTERRUPT_TYPE_TRAP, 0x07);
    installInterrupt((uint64_t)&doubleException, GDT_KERNEL_CODE, PRIVILEGE_KERNEL, INTERRUPT_TYPE_TRAP, 0x08);
    installInterrupt((uint64_t)&tssException, GDT_KERNEL_CODE, PRIVILEGE_KERNEL, INTERRUPT_TYPE_TRAP, 0x0A);
    installInterrupt((uint64_t)&segmentException, GDT_KERNEL_CODE, PRIVILEGE_KERNEL, INTERRUPT_TYPE_TRAP, 0x0B);
    installInterrupt((uint64_t)&stackException, GDT_KERNEL_CODE, PRIVILEGE_KERNEL, INTERRUPT_TYPE_TRAP, 0x0C);
    installInterrupt((uint64_t)&gpException, GDT_KERNEL_CODE, PRIVILEGE_KERNEL, INTERRUPT_TYPE_TRAP, 0x0D);
    installInterrupt((uint64_t)&pageException, GDT_KERNEL_CODE, PRIVILEGE_KERNEL, INTERRUPT_TYPE_TRAP, 0x0E);
    installInterrupt((uint64_t)&mathException, GDT_KERNEL_CODE, PRIVILEGE_KERNEL, INTERRUPT_TYPE_TRAP, 0x10);
    installInterrupt((uint64_t)&alignmentException, GDT_KERNEL_CODE, PRIVILEGE_KERNEL, INTERRUPT_TYPE_TRAP, 0x11);
    installInterrupt((uint64_t)&machineCheckException, GDT_KERNEL_CODE, PRIVILEGE_KERNEL, INTERRUPT_TYPE_TRAP, 0x12);
    installInterrupt((uint64_t)&xmathException, GDT_KERNEL_CODE, PRIVILEGE_KERNEL, INTERRUPT_TYPE_TRAP, 0x13);
    installInterrupt((uint64_t)&virtualException, GDT_KERNEL_CODE, PRIVILEGE_KERNEL, INTERRUPT_TYPE_TRAP, 0x14);
    installInterrupt((uint64_t)&controlException, GDT_KERNEL_CODE, PRIVILEGE_KERNEL, INTERRUPT_TYPE_TRAP, 0x15);

    // simulate a page fault for testing
    uint64_t *ptr = (uint64_t *)0x78AF748392;   // some invalid address
    KDEBUG("page fault: %d\n", *ptr);
}

void exception(uint64_t number, uint64_t code) {
    // TODO: handle different exceptions differently (specifically page faults)
    // TODO: implement a separaye kernel panic and userspace exception handling
    KERROR("%d - %s with error code %d\n", number, exceptions[number], code);
    while(1);
}