
; lux - a lightweight unix-like operating system
; Omar Elghoul, 2024

[bits 64]

section .text

%include "cpu/stack.asm"

; Stub for IRQ Handlers
extern dispatchIRQ

%macro irq 2
global %1
align 16
%1:
    cli
    pushaq
    mov rdi, %2     ; irq number
    cld
    call dispatchIRQ
    popaq
    iretq
%endmacro

irq     dispatchIRQ0, 0
irq     dispatchIRQ1, 1
irq     dispatchIRQ2, 2
irq     dispatchIRQ3, 3
irq     dispatchIRQ4, 4
irq     dispatchIRQ5, 5
irq     dispatchIRQ6, 6
irq     dispatchIRQ7, 7
irq     dispatchIRQ8, 8
irq     dispatchIRQ9, 9
irq     dispatchIRQ10, 10
irq     dispatchIRQ11, 11
irq     dispatchIRQ12, 12
irq     dispatchIRQ13, 13
irq     dispatchIRQ14, 14
irq     dispatchIRQ15, 15
irq     dispatchIRQ16, 16
irq     dispatchIRQ17, 17
irq     dispatchIRQ18, 18
irq     dispatchIRQ19, 19
irq     dispatchIRQ20, 20
irq     dispatchIRQ21, 21
irq     dispatchIRQ22, 22
irq     dispatchIRQ23, 23
irq     dispatchIRQ24, 24
irq     dispatchIRQ25, 25
irq     dispatchIRQ26, 26
irq     dispatchIRQ27, 27
irq     dispatchIRQ28, 28
irq     dispatchIRQ29, 29
irq     dispatchIRQ30, 30
irq     dispatchIRQ31, 31
irq     dispatchIRQ32, 32
irq     dispatchIRQ33, 33
irq     dispatchIRQ34, 34
irq     dispatchIRQ35, 35
irq     dispatchIRQ36, 36
irq     dispatchIRQ37, 37
irq     dispatchIRQ38, 38
irq     dispatchIRQ39, 39
irq     dispatchIRQ40, 40
irq     dispatchIRQ41, 41
irq     dispatchIRQ42, 42
irq     dispatchIRQ43, 43
irq     dispatchIRQ44, 44
irq     dispatchIRQ45, 45
irq     dispatchIRQ46, 46
irq     dispatchIRQ47, 47

section .rodata

global dispatchIRQTable
align 16
dispatchIRQTable:
    dq dispatchIRQ0, dispatchIRQ1, dispatchIRQ2, dispatchIRQ3, dispatchIRQ4, dispatchIRQ5
    dq dispatchIRQ6, dispatchIRQ7, dispatchIRQ8, dispatchIRQ9, dispatchIRQ10, dispatchIRQ11
    dq dispatchIRQ12, dispatchIRQ13, dispatchIRQ14, dispatchIRQ15, dispatchIRQ16, dispatchIRQ17
    dq dispatchIRQ18, dispatchIRQ19, dispatchIRQ20, dispatchIRQ21, dispatchIRQ22, dispatchIRQ23
    dq dispatchIRQ24, dispatchIRQ25, dispatchIRQ26, dispatchIRQ27, dispatchIRQ28, dispatchIRQ29
    dq dispatchIRQ30, dispatchIRQ31, dispatchIRQ32, dispatchIRQ33, dispatchIRQ34, dispatchIRQ35
    dq dispatchIRQ36, dispatchIRQ37, dispatchIRQ38, dispatchIRQ39, dispatchIRQ40, dispatchIRQ41
    dq dispatchIRQ42, dispatchIRQ43, dispatchIRQ44, dispatchIRQ45, dispatchIRQ46, dispatchIRQ47
