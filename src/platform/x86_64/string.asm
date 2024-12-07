
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

; void *memmove(void *dst, const void *src, size_t n)
global memmove
align 16
memmove:
    mov rax, rdi        ; return value

    mov r8, rdi
    sub r8, rsi         ; dst-src
    pushfq
    jns .check_overlap

    not r8
    inc r8

.check_overlap:
    cmp r8, 8
    jge .memcpy

    popfq
    jns .backward        ; if dst>src copy backwards

.forward:
    mov rcx, rdx
    rep movsb

    ret

.backward:
    std
    mov rcx, rdx
    dec rdx
    add rdi, rdx
    add rsi, rdx
    rep movsb
    cld

    ret

.memcpy:
    popfq
    jmp memcpy

; void *memset(void *dst, int val, size_t n)
global memset
align 16
memset:
    mov r8, rdi         ; r8 = dst

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
    or rax, rsi         ; full 64 bits

    mov rcx, rdx
    shr rcx, 3          ; div 8
    rep stosq

    and rdx, 7          ; mod 8

.low:
    mov rcx, rdx
    rep stosb

    mov rax, r8         ; return value
    ret
