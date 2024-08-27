
; lux - a lightweight unix-like operating system
; Omar Elghoul, 2024

[bits 64]

section .text

; Context Switching for the Scheduler

; platformSaveContext(): saves the context of the current running thread
; platformSaveContext(ThreadContext *, ThreadGPR *)
; sizeof(ThreadGPR) = 160
; sizeof(ThreadContext) = 680
; SSE offset = 0
; CR3 offset = 512
; ThreadGPR offset = 520

global platformSaveContext
align 16
platformSaveContext:
    mov rax, cr3
    mov [rdi+512], rax

    add rdi, 520
    mov rcx, 160/8
    cld
    rep movsq

    ret

