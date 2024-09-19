
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

; void *memset(void *dst, int val, size_t n)
global memset
align 16
memset:
    mov r8, rdx         ; r8 = count

    and rsi, 0xFF
    mov rax, rsi        ; low 8 bits

    cmp rdx, 8
    jl .low

    shr rax, 8
    or rax, rsi         ; 16 bits
    mov si, ax
    shr rax, 16
    mov ax, si          ; 32 bits
    mov esi, eax
    shr rax, 32
    mov eax, esi        ; full 64 bits

    mov rcx, rdx
    shr rcx, 3          ; div 8
    rep stosq

    and rdx, 7          ; mod 8

.low:
    mov rcx, rdx
    rep stosb

    mov rax, rdi
    sub rax, r8
    ret
