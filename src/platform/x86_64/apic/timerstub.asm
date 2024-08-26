
; lux - a lightweight unix-like operating system
; Omar Elghoul, 2024

[bits 64]

section .text

%include "cpu/stack.asm"

; assembly stub for the timer IRQ handler
; just to preserve the CPU state and then call the true handler

global timerHandlerStub
align 16
timerHandlerStub:
    pushaq

    cld
    extern timerIRQ     ; apic.c
    call timerIRQ       ; IRQ is acknowledged in here

    popaq
    iretq
