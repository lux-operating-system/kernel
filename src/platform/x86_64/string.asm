
; lux - a lightweight unix-like operating system
; Omar Elghoul, 2024

[bits 64]

section .text

; void *memcpy(void *dst, const void *src, size_t n)
global memcpy
align 16
memcpy:
    mov rax, rdi        ; return value

    cmp rdx, 8
    jl .low

    mov rcx, rdx
    shr rcx, 3          ; div 8
    rep movsq

    and rdx, 7          ; mod 8

.low:
    mov rcx, rdx
    rep movsb

    ret
