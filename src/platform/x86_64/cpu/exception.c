/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Platform-Specific Code for x86_64
 */

#include <platform/x86_64.h>
#include <platform/exception.h>
#include <platform/platform.h>
#include <platform/lock.h>
#include <kernel/logger.h>
#include <kernel/memory.h>
#include <kernel/sched.h>

static lock_t lock = LOCK_INITIAL;

static const char *exceptions[] = {
    "divide error",             // 0x00
    "debug exception",          // 0x01
    "non-maskable interrupt",   // 0x02
    "breakpoint",               // 0x03
    "overflow",                 // 0x04
    "boundary range exceeded",  // 0x05
    "undefined opcode",         // 0x06
    "device not present",       // 0x07
    "double fault",             // 0x08
    "reserved exception",       // 0x09
    "invalid TSS",              // 0x0A
    "data segment exception",   // 0x0B
    "stack segment exception",  // 0x0C
    "general protection fault", // 0x0D
    "page fault",               // 0x0E
    "reserved exception",       // 0x0F
    "math fault",               // 0x10
    "alignment exception",      // 0x11
    "machine check fail",       // 0x12
    "extended math fault",      // 0x13
    "virtualization fault",     // 0x14
    "control protection fault", // 0x15
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
}

void exception(uint64_t number, uint64_t code, InterruptRegisters *r) {
    // TODO: handle different exceptions differently

    // invoke the virtual memory manager ONLY if a page fault occurs for an ABSENT page
    // because this will either mean that a page needs to be swapped OR physical memory
    // needs to be allocated
    // page faults for PRESENT pages mean that a thread violated its permissions, so
    // terminate the process (TODO)
    if((number == 14) && !(code & PF_PRESENT)) {     // page fault is exception #14 on x86
        int pfStatus = 0;
        if(code & PF_FETCH) pfStatus |= VMM_PAGE_FAULT_FETCH;
        if(code & PF_USER) pfStatus |= VMM_PAGE_FAULT_USER;
        if(code & PF_WRITE) pfStatus |= VMM_PAGE_FAULT_WRITE;
        if(code & PF_FETCH) pfStatus |= VMM_PAGE_FAULT_FETCH;

        uintptr_t addr = readCR2();
        if(!vmmPageFault(addr, pfStatus)) return;
    }

    // TODO: implement a separate kernel panic and userspace exception handling
    acquireLockBlocking(&lock);
    schedLock();
    pid_t pid = getPid();
    if(pid > 0) {
        pid_t tid = getTid();
        Thread *t = getThread(getTid());
        if(number == 14) {
            // for unresolved page faults, print out the faulting logical address
            KWARN("cpu %d (tid %d): %s @ 0x%X:0x%X, code %d address 0x%X\n", platformWhichCPU(), tid, exceptions[number], r->cs, r->rip, code, readCR2());
            if(t) KWARN("program break was at 0x%X\n", t->highest);
        } else {
            // for everything but page faults
            KWARN("cpu %d (tid %d): %s @ 0x%X:0x%X, code %d\n", platformWhichCPU(), tid, exceptions[number], r->cs, r->rip, code);
        }

        // and let the scheduler handle it
        if(schedException(pid, tid)) {
            // here the faulty thread was terminated
            releaseLock(&lock);
            schedRelease();
            for(;;) schedule();
        } else {
            // handled and can continue
            releaseLock(&lock);
            schedRelease();
            return;
        }
    }

    KPANIC("kernel panic: cpu %d: %s, code %d\n", platformWhichCPU(), exceptions[number], code);
    KPANIC(" rip: 0x%016X  cs:  0x%02X\n", r->rip, r->cs);
    KPANIC(" rax: 0x%016X  rbx: 0x%016X  rcx: 0x%016X\n", r->rax, r->rbx, r->rcx);
    KPANIC(" rdx: 0x%016X  rsi: 0x%016X  rdi: 0x%016X\n", r->rdx, r->rsi, r->rdi);
    KPANIC(" r8:  0x%016X  r9:  0x%016X  r10: 0x%016X\n", r->r8, r->r9, r->r10);
    KPANIC(" r11: 0x%016X  r12: 0x%016X  r13: 0x%016X\n", r->r11, r->r12, r->r13);
    KPANIC(" r14: 0x%016X  r15: 0x%016X\n", r->r14, r->r15);
    KPANIC(" rsp: 0x%016X  rbp: 0x%016X  ss: 0x%02X\n", r->rsp, r->rbp, r->ss);
    KPANIC(" cr2: 0x%016X  cr3: 0x%016X\n", readCR2(), readCR3());
    KPANIC(" cr0: 0x%08X  cr4: 0x%08X  rflags: 0x%08X\n", readCR0(), readCR4(), r->rflags);

    schedRelease();
    releaseLock(&lock);
    while(1);
}