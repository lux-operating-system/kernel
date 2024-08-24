
; lux - a lightweight unix-like operating system
; Omar Elghoul, 2024

; This is the entry point that runs when other CPUs are started up
; All x86 CPUs start in 16-bit mode, so this code has to set up and enable
; 64-bit long mode

[bits 16]

section .data
global apEntry
align 16
apEntry:
    ; this code will actually start running at 0x1000; the kernel will copy it
    ; into low memory for accessibility from 16-bit mode
    xor ax, ax
    mov ds, ax

    mov eax, [0x1FE4]
    lgdt [eax]
    mov eax, [0x1FE8]
    lidt [eax]
    mov eax, [0x1FEC]
    mov cr3, eax

    ; configure the CPU for long mode - this is the same sequence we followed
    ; in the boot loader
    mov eax, 0x6A0          ; enable SSE, PAE, and global pages
    mov cr4, eax

    mov ecx, 0xC0000080
    rdmsr
    or eax, 0x100           ; enable 64-bit mode
    wrmsr

    mov eax, cr0
    and eax, 0x9FFFFFFF     ; enable global caching
    or eax, 0x80010001      ; enable paging, write-protection, and protected mode
    mov cr0, eax

    jmp 0x08:0x1080

times 0x80 - ($-$$) nop
[bits 64]

    ; now we're in 64-bit long mode
    mov rax, 0x10
    mov ss, rax
    mov ds, rax
    mov es, rax
    mov fs, rax
    mov gs, rax

    mov rax, 0x1FF0         ; top of stack pointer
    mov rsp, [rax]
    mov rbp, rsp

    mov rax, 2
    push rax
    popfq

    ; indicate to the kernel that the CPU started
    mov eax, 1
    mov [0x1FE0], eax

    mov rax, 0x1FF8         ; entry point
    mov rax, [rax]
    call rax

    ; this should never return

    times 0x100 - ($-$$) nop

.hang:
    cli
    hlt
    jmp 0x1100

times 0xFE0 - ($-$$) nop

global apEntryVars
apEntryVars:

; the kernel will also fill in these variables
life:               dd 0        ; 0x1FE0: sign of life
gdtrPointer:        dd 0        ; 0x1FE4: pointer to the GDTR
idtrPointer:        dd 0        ; 0x1FE8: pointer to the IDTR
kernelCR3:          dd 0        ; 0x1FEC: pointer to kernel paging
stackTopLow:        dd 0        ; 0x1FF0: newly allocated stack for this CPU
stackTopHigh:       dd 0        ; 0x1FF4: high bits
nextLow:            dd 0        ; 0x1FF8: true entry point to call next
nextHigh:           dd 0        ; 0x1FFC: high bits

global apEntryEnd
apEntryEnd:
