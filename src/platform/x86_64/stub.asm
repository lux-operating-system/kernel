
; lux - a lightweight unix-like operating system
; Omar Elghoul, 2024

[bits 64]

section .text
global _start

; Kernel entry point stub from boot loader
; prototype: void _start(KernelBootInfo *k)
align 8
_start:
    mov rsp, initial_stack_top
    mov rbp, rsp

    ; all remaining CPU setup will be done in the main C code
    extern main
    cld
    mov rax, main
    call rax

    ; this should never return
.hang:
    cli
    hlt
    jmp .hang

section .bss

align 32
initial_stack:   resb 16384
initial_stack_top:
