
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

    ; now we call platform-specific initialization code
    ; this function will later call the true kernel main
    extern platformMain
    cld
    mov rax, platformMain
    call rax

.hang:
    ; this should never return
    cli
    hlt
    jmp .hang

section .bss

align 32
initial_stack:   resb 16384
initial_stack_top:
