
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
    mov eax, [rdi]
    and eax, 1
    ret

; int acquireLock(lock_t *lock)
; non-blocking function to acquire a lock
; returns zero if not acquired, one if acquired
global acquireLock
align 16
acquireLock:
    pushfq
    cli

    lock bts dword [rdi], 0
    jc .fail

    popfq
    mov rax, 1
    ret

.fail:
    popfq
    xor rax, rax
    ret

; int acquireLockBlocking(lock_t *lock)
; blocking function to acquire lock, always returns one
global acquireLockBlocking
align 16
acquireLockBlocking:
    pushfq
    cli

.try:
    lock bts dword [rdi], 0
    jc .wait

    popfq
    mov rax, 1
    ret

.wait:
    pause
    test dword [rdi], 1
    jnz .wait
    jmp .try

; int releaseLock(lock_t *lock)
; releases a lock, always returns zero
global releaseLock
align 16
releaseLock:
    pushfq
    cli

    xor rax, rax
    test dword [rdi], 1
    jz .done

    mov [rdi], eax

.done:
    popfq
    ret
