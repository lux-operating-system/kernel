
; lux - a lightweight unix-like operating system
; Omar Elghoul, 2024

[bits 64]

; Signal Trampoline Code
; this simply invokes the sigreturn() system call to allow the kernel to
; restore the state of the thread before the signal was raised

SYSCALL_SIGRETURN           equ 49

section .data

global sigstub
align 16
sigstub:
    mov rax, SYSCALL_SIGRETURN
    syscall

sigstubEnd:

global sigstubSize
align 16
sigstubSize:                dq sigstubEnd - sigstub
