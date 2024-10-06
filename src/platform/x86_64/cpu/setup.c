/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

#include <string.h>
#include <platform/platform.h>
#include <platform/gdt.h>
#include <platform/idt.h>
#include <platform/x86_64.h>

GDTEntry gdt[GDT_ENTRIES];
GDTR gdtr;
IDTEntry idt[256];
IDTR idtr;

static char _model[49];
char *platformCPUModel = _model;

int platformCPUSetup() {
    // construct a global descriptor table
    memset(&gdt, 0, sizeof(GDTEntry) * GDT_ENTRIES);

    // kernel code descriptor
    gdt[GDT_KERNEL_CODE].baseLo = 0;
    gdt[GDT_KERNEL_CODE].baseMi = 0;
    gdt[GDT_KERNEL_CODE].baseHi = 0;
    gdt[GDT_KERNEL_CODE].limit = 0xFFFF;
    gdt[GDT_KERNEL_CODE].flagsLimitHi = GDT_FLAGS_64_BIT | GDT_FLAGS_PAGE_GRAN | 0x0F;  // limit high
    gdt[GDT_KERNEL_CODE].access = GDT_ACCESS_PRESENT | GDT_ACCESS_CODE_DATA | GDT_ACCESS_EXEC | GDT_ACCESS_RW;

    // kernel data descriptor
    gdt[GDT_KERNEL_DATA].baseLo = 0;
    gdt[GDT_KERNEL_DATA].baseMi = 0;
    gdt[GDT_KERNEL_DATA].baseHi = 0;
    gdt[GDT_KERNEL_DATA].limit = 0xFFFF;
    gdt[GDT_KERNEL_DATA].flagsLimitHi = GDT_FLAGS_PAGE_GRAN | 0x0F;  // limit high
    gdt[GDT_KERNEL_DATA].access = GDT_ACCESS_PRESENT | GDT_ACCESS_CODE_DATA | GDT_ACCESS_RW;

    // user code descriptor
    gdt[GDT_USER_CODE].baseLo = 0;
    gdt[GDT_USER_CODE].baseMi = 0;
    gdt[GDT_USER_CODE].baseHi = 0;
    gdt[GDT_USER_CODE].limit = 0xFFFF;
    gdt[GDT_USER_CODE].flagsLimitHi = GDT_FLAGS_64_BIT | GDT_FLAGS_PAGE_GRAN | 0x0F;  // limit high
    gdt[GDT_USER_CODE].access = GDT_ACCESS_PRESENT | GDT_ACCESS_CODE_DATA | GDT_ACCESS_EXEC | GDT_ACCESS_RW;
    gdt[GDT_USER_CODE].access |= (GDT_ACCESS_DPL_USER << GDT_ACCESS_DPL_SHIFT);

    // user data descriptor
    gdt[GDT_USER_DATA].baseLo = 0;
    gdt[GDT_USER_DATA].baseMi = 0;
    gdt[GDT_USER_DATA].baseHi = 0;
    gdt[GDT_USER_DATA].limit = 0xFFFF;
    gdt[GDT_USER_DATA].flagsLimitHi = GDT_FLAGS_PAGE_GRAN | 0x0F;  // limit high
    gdt[GDT_USER_DATA].access = GDT_ACCESS_PRESENT | GDT_ACCESS_CODE_DATA | GDT_ACCESS_RW;
    gdt[GDT_USER_DATA].access |= (GDT_ACCESS_DPL_USER << GDT_ACCESS_DPL_SHIFT);

    // load the GDT
    gdtr.base = (uint64_t)&gdt;
    gdtr.limit = (sizeof(GDTEntry) * GDT_ENTRIES) - 1;
    loadGDT(&gdtr);
    resetSegments(GDT_KERNEL_CODE, PRIVILEGE_KERNEL);

    // create and load an empty interrupt descriptor table
    memset(&idt, 0, sizeof(IDTEntry) * 256);
    idtr.base = (uint64_t)&idt;
    idtr.limit = (sizeof(IDTEntry) * 256) - 1;
    loadIDT(&idtr);

    writeCR0(readCR0() & ~CR0_NOT_WRITE_THROUGH);
    writeCR0(readCR0() & ~CR0_CACHE_DISABLE);
    writeCR0(readCR0() & ~CR0_WRITE_PROTECT);

    // read the CPU model
    memset(_model, 0, 49);
    uint32_t *ptr = (uint32_t *) _model;
    CPUIDRegisters regs;
    readCPUID(0x80000000, &regs);

    if(regs.eax < 0x80000004) {
        readCPUID(0, &regs);
        ptr[0] = regs.ebx;
        ptr[1] = regs.edx;
        ptr[2] = regs.ecx;
    } else {
        readCPUID(0x80000002, &regs);
        ptr[0] = regs.eax;
        ptr[1] = regs.ebx;
        ptr[2] = regs.ecx;
        ptr[3] = regs.edx;

        readCPUID(0x80000003, &regs);
        ptr[4] = regs.eax;
        ptr[5] = regs.ebx;
        ptr[6] = regs.ecx;
        ptr[7] = regs.edx;

        readCPUID(0x80000004, &regs);
        ptr[8] = regs.eax;
        ptr[9] = regs.ebx;
        ptr[10] = regs.ecx;
        ptr[11] = regs.edx;
    }

    enableIRQs();
    return 0;
}