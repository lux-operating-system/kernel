
; lux - a lightweight unix-like operating system
; Omar Elghoul, 2024

[bits 64]

; Wrappers for instructions that are not directly usable in higher-level code

section .text

global readCR0
align 16
readCR0:
    mov rax, cr0
    ret

global writeCR0
align 16
writeCR0:
    mov cr0, rdi
    ret

global readCR2
align 16
readCR2:
    mov rax, cr2
    ret

global readCR3
align 16
readCR3:
    mov rax, cr3
    ret

global writeCR3
align 16
writeCR3:
    mov cr3, rdi
    ret

global readCR4
align 16
readCR4:
    mov rax, cr4
    ret

global writeCR4
align 16
writeCR4:
    mov cr4, rdi
    ret

global loadGDT
align 16
loadGDT:
    lgdt [rdi]
    ret

global loadIDT
align 16
loadIDT:
    lidt [rdi]
    ret

global outb
align 16
outb:
    mov rdx, rdi
    mov rax, rsi
    out dx, al
    ret

global outw
align 16
outw:
    mov rdx, rdi
    mov rax, rsi
    out dx, ax
    ret

global outd
align 16
outd:
    mov rdx, rdi
    mov rax, rsi
    out dx, eax
    ret

global inb
align 16
inb:
    mov rdx, rdi
    in al, dx
    ret

global inw
align 16
inw:
    mov rdx, rdi
    in ax, dx
    ret

global ind
align 16
ind:
    mov rdx, rdi
    in eax, dx
    ret

global resetSegments
align 16
resetSegments:
    mov rax, rsp    ; preserve original stack

    ; rdi = index into GDT for code segment
    ; rsi = privilege level
    inc rdi         ; advance to data segment
    shl rdi, 3      ; mul 8
    or rdi, rsi

    mov ds, rdi
    mov es, rdi
    mov fs, rdi
    mov gs, rdi

    ; create interrupt frame
    push rdi        ; ss
    push rax        ; rsp
    pushfq          ; rflags
    
    sub rdi, 8
    push rdi        ; code segment
    mov rax, .next
    push rax        ; rip
    iretq

.next:
    ret

global readCPUID
align 16
readCPUID:
    ; readCPUID(uint32_t leaf, CPUIDRegisters *regs)
    ; typedef struct {
    ;   uint32_t eax;
    ;   uint32_t ebx;
    ;   uint32_t ecx;
    ;   uint32_t edx;
    ; } __attribute__((packed)) CPUIDRegisters;

    push rbx

    mov eax, edi        ; leaf
    push rsi            ; regs
    mov ecx, [esi+8]
    cpuid
    pop rsi
    mov [rsi], eax
    mov [rsi+4], ebx
    mov [rsi+8], ecx
    mov [rsi+12], edx

    pop rbx

    ret
