
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
    cli
    pushaq

    cld
    extern timerIRQ     ; apic.c
    mov rdi, rsp        ; pointer to the regs we just pushed
    sub rsp, 256        ; red zone
    mov rbp, rsp
    call timerIRQ       ; IRQ is acknowledged in here

    add rbp, 256
    mov rsp, rbp

    popaq
    iretq
