/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Platform-Specific Code for x86_64
 */

#include <string.h>
#include <stdlib.h>
#include <platform/x86_64.h>
#include <platform/tss.h>
#include <kernel/logger.h>

/* tssSetup(): sets up a task state segment in the GDT for each CPU */

void tssSetup() {
    GDTR gdtr;
    storeGDT(&gdtr);

    GDTEntry *gdt = malloc(gdtr.limit+1);
    if(!gdt) {
        KERROR("failed to allocate memory for GDT\n");
        while(1);
    }

    memcpy(gdt, (void *)gdtr.base, gdtr.limit+1);
    gdtr.base = (uint64_t)gdt;

    // now create a TSS
    TSS *tss = calloc(1, sizeof(TSS));
    if(!tss) {
        KERROR("failed to allocate memory for TSS\n");
        while(1);
    }
    
    void *stack = calloc(1, KENREL_STACK_SIZE);
    if(!stack) {
        KERROR("failed to allocate memory for kernel stack\n");
        while(1);
    }

    tss->rsp0 = (uint64_t)stack + KENREL_STACK_SIZE;
    tss->rsp1 = (uint64_t)stack + KENREL_STACK_SIZE;
    tss->rsp2 = (uint64_t)stack + KENREL_STACK_SIZE;

    gdt[GDT_TSS_LOW].baseLo = (uint16_t)tss;
    gdt[GDT_TSS_LOW].baseMi = (uint8_t)tss >> 16;
    gdt[GDT_TSS_LOW].baseHi = (uint8_t)tss >> 24;
    gdt[GDT_TSS_LOW].limit = sizeof(TSS);
    gdt[GDT_TSS_LOW].access = GDT_ACCESS_TSS | GDT_ACCESS_PRESENT;
    gdt[GDT_TSS_LOW].access |= (GDT_ACCESS_DPL_USER << GDT_ACCESS_DPL_SHIFT);

    uint64_t *high = (uint64_t *)&gdt[GDT_TSS_HIGH];
    *high = (uint64_t)tss >> 32;

    loadGDT(&gdtr);
    loadTSS((GDT_TSS_LOW << 3) | PRIVILEGE_USER);
}