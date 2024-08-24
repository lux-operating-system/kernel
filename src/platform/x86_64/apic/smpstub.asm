
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

    mov eax, [0x1FF0]
    lgdt [eax]
    mov eax, [0x1FF4]
    lidt [eax]
    mov eax, [0x1FF8]
    mov cr3, eax

    cli
    hlt

times 0x1FF0 - ($-$$) db 0

global apEntryVars:
apEntryVars:

; the kernel will also fill in these variables
gdtrPointer:        dd 0        ; 0x1FF0: pointer to the GDTR
idtrPointer:        dd 0        ; 0x1FF4: pointer to the IDTR
kernelCR3:          dd 0        ; 0x1FF8: pointer to kernel paging
stackTop:           dd 0        ; 0x1FFC: newly allocated stack for this CPU

global apEntryEnd
apEntryEnd:
