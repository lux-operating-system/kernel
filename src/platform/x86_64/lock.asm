
; lux - a lightweight unix-like operating system
; Omar Elghoul, 2024

[bits 64]

section .text

; Implementation of Locks

; int lockStatus(lock_t *lock)
; returns zero if the lock is free, one if not
global lockStatus
align 16
lockStatus:
    xor rax, rax
    mov al, byte[rdi]
    and al, al
    jz .done
    mov al, 1

.done:
    ret

; int acquireLock(lock_t *lock)
; non-blocking function to acquire a lock
; returns zero if not acquired, one if acquired
global acquireLock
align 16
acquireLock:
    lock bts word[rdi], 0
    jc .fail

    mov rax, 1
    ret

.fail:
    xor rax, rax
    ret

; int acquireLockBlocking(lock_t *lock)
; blocking function to acquire lock, always returns one
global acquireLockBlocking
align 16
acquireLockBlocking:
    lock bts word[rdi], 0
    jc .wait

    mov rax, 1
    ret

.wait:
    pause
    bt word [rdi], 0
    jc .wait
    jmp acquireLockBlocking

; int releaseLock(lock_t *lock)
; releases a lock, always returns zero
global releaseLock
align 16
releaseLock:
    lock btr word[rdi], 0
    xor rax, rax
    ret
