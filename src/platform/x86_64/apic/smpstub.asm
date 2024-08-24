
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

    ; indicate to the kernel that the CPU started
    mov eax, 1
    mov [0x1FE0], eax

    cli
    hlt

times 0xFE0 - ($-$$) db 0

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
