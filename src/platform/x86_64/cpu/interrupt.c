/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Platform-Specific Code for x86_64
 */

#include <platform/x86_64.h>
#include <kernel/logger.h>

/* installInterrupt(): installs an interrupt handler
 * params: handler - pointer to the entry point of the handler
 * params: segment - segment within the GDT to be loaded on execution
 * params: privilege - user or kernel privilege
 * params: type - interrupt or trap
 * params: i - interrupt number
 * returns: nothing
 */

void installInterrupt(uint64_t handler, uint16_t segment, int privilege, int type, int i) {
    if(i > 0xFF) {
        KERROR("request to install interrupt handler %d, ignoring\n", i);
        return;
    }

    privilege &= 3;
    type &= 0x0F;

    // this is really just about creating an IDT entry
    idt[i].offset_lo = handler & 0xFFFF;
    idt[i].offset_mi = (handler >> 16) & 0xFFFF;
    idt[i].offset_hi = handler >> 32;
    idt[i].segment = (segment << 3) | privilege;
    idt[i].flags = (privilege << IDT_FLAGS_DPL_SHIFT) | (type << IDT_FLAGS_TYPE_SHIFT);
    idt[i].flags |= IDT_FLAGS_VALID;
}