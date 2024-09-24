
; lux - a lightweight unix-like operating system
; Omar Elghoul, 2024

[bits 64]

section .text

%include "cpu/stack.asm"

; For the kernel idle thread
; void platformIdle()
global platformIdle
align 16
platformIdle:
    cli                 ; sensitive area

    sub rsp, 40         ; reserve iret frame
    pushaq

    mov rbp, rsp        ; rsp to be restored

    add rsp, 160        ; skip over the pushed reg state

    push qword 0x10     ; ss
    push rbp            ; rsp
    pushfq
    pop r10
    or r10, 0x202
    push r10            ; rflags
    push qword 0x08     ; cs
    mov r10, .next
    push r10            ; rip

    ; save this stack frame on the kernel's stack
    rdgsbase r10
    and r10, r10
    jnz .stack

    swapgs
    rdgsbase r10

.stack:
    swapgs

    mov r10, [r10+8]    ; r10 = top of kernel switch stack
    mov rdi, r10
    sub rdi, 160        ; rdi = bottom of kernel stack frame
    mov rsi, rbp        ; rsi = stack we just created
    mov rcx, 160/8
    rep movsq

    ; switch stacks
    sub rdi, 160        ; rdi = bottom of kernel stack frame
    mov rsp, rdi
    sub rsp, 256

    extern kernelYield
    call kernelYield

    mov rsp, rbp

    ; if we get here then there is no work in the scheduler's queue
    ; we need to restore the original stack
    ; so halt the CPU for one timer tick so it doesn't burn up
    sti
    hlt

.next:
    popaq
    add rsp, 40         ; skip over iret frame
    ret
