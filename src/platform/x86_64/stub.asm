[bits 64]

section .text
global _start

_start:
    cli
    hlt
    jmp _start

section .data

something:      dd 0
something_else: dq 189


section .bss

somethingfff:   resb 10000