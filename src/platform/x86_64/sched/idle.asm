
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

    mov r8, rsp         ; rsp to be restored

    add rsp, 160        ; skip over the pushed reg state

    push qword 0x10     ; ss
    push r8             ; rsp
    pushfq
    pop r10
    or r10, 0x202
    push r10            ; rflags
    push qword 0x08     ; cs
    mov r10, .next
    push r10            ; rip

    mov rsp, r8         ; restore stack frame
    mov rdi, rsp        ; and pass it as a parameter

    extern kernelYield  ; check for other queued threads
    call kernelYield

    ; if we get here then there is no work in the scheduler's queue
    ; so halt the CPU for one timer tick so it doesn't burn up
    sti
    hlt

.next:
    popaq
    add rsp, 40         ; skip over iret frame
    ret
