
; lux - a lightweight unix-like operating system
; Omar Elghoul, 2024

[bits 64]

section .text

%include "cpu/stack.asm"

USER_CODE_SEGMENT           equ 0x23    ; gdt.h, these are hard-coded for a reason
USER_DATA_SEGMENT           equ 0x1B

; entry point into the kernel by the SYSCALL instruction
; this saves many CPU cycles compared to implementing syscalls via interrupts

global syscallEntry
align 16
syscallEntry:
    ; rcx = return address for user program
    ; r11 = rflags for user program
    
    ; switch to kernel stack
    mov r9, rsp                 ; r9 = user stack
    rdgsbase r10                ; a register we're allowed to trash
    and r10, r10
    jnz .stack

    swapgs
    rdgsbase r10                ; r10 = pointer to kernel cpu-specific info block

.stack:
    swapgs                      ; restore base 0 segment

    mov r10, [r10]
    mov rsp, r10

    ; construct a context switch stack as if an interrupt just happened
    push qword USER_DATA_SEGMENT    ; ss
    push r9                         ; rsp
    push r11                        ; rflags
    push qword USER_CODE_SEGMENT    ; cs
    push rcx                        ; rip

    ;xor r9, r9
    ;xor r11, r11
    pushaq

    extern syscallHandle
    mov rdi, rsp                    ; pass the context we just created
    call syscallHandle

    ; this should never return as it will force a context switch

.hang:
    cli
    hlt
    jmp .hang
