
; lux - a lightweight unix-like operating system
; Omar Elghoul, 2024

[bits 64]

section .text

%include "cpu/stack.asm"

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
    cli         ; SENSITIVE AREA

    fxsave64 [rdi]

    ;mov rax, cr3
    ;mov [rdi+512], rax

    add rdi, 520
    mov rcx, 160/8
    cld
    rep movsq

    ret

; platformLoadContext(): loads the new context
; platformLoadContext(ThreadContext *)

global platformLoadContext:
align 16
platformLoadContext:
    cli         ;; SENSITIVE!!! this code can NOT be interrupted

    fxrstor64 [rdi]

    ; save performance by only invalidating TLB if the context actually changed
    mov rax, cr3
    mov rbx, [rdi+512]      ; pml4
    cmp rax, rbx
    jz .continue

    mov cr3, rbx

.continue:
    mov rax, [rdi+672]      ; stack segment
    mov ds, rax
    mov es, rax
    mov fs, rax
    mov gs, rax

    mov rsp, rdi
    add rsp, 520

    popaq
    iretq
