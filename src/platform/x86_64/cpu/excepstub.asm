
; lux - a lightweight unix-like operating system
; Omar Elghoul, 2024

[bits 64]

; Stubs for exception handlers

section .text

%macro pushaq 0
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
%endmacro

%macro popaq 0
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
%endmacro

extern exception

; handler_no_code name, number
%macro handler_no_code 2
global %1
align 16
%1:
    push 0                  ; fake code
    pushaq
    mov rdi, %2
    mov rsi, 0              ; fake code
    mov rdx, rsp            ; registers we just pushed
    cld
    call exception
    popaq
    iretq
%endmacro

; handler_code name, number
%macro handler_code 2
global %1
align 16
%1:
    pushaq
    mov rdi, %2
    mov rsi, [rsp+120]      ; error code
    mov rdx, rsp            ; registers we just pushed
    cld
    call exception
    popaq
    iretq
%endmacro

handler_no_code     divideException, 0x00
handler_no_code     debugException, 0x01
handler_no_code     nmiException, 0x02
handler_no_code     breakpointException, 0x03
handler_no_code     overflowException, 0x04
handler_no_code     boundaryException, 0x05
handler_no_code     opcodeException, 0x06
handler_no_code     deviceException, 0x07
handler_code        doubleException, 0x08
handler_code        tssException, 0x0A
handler_code        segmentException, 0x0B
handler_code        stackException, 0x0C
handler_code        gpException, 0x0D
handler_code        pageException, 0x0E
handler_no_code     mathException, 0x10
handler_no_code     alignmentException, 0x11
handler_no_code     machineCheckException, 0x12
handler_no_code     xmathException, 0x13
handler_no_code     virtualException, 0x14
handler_code        controlException, 0x15
