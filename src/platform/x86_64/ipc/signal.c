/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Platform-Specific Code for x86_64
 */

#include <stdlib.h>
#include <string.h>
#include <platform/platform.h>
#include <platform/context.h>
#include <kernel/signal.h>
#include <kernel/sched.h>

extern size_t sigstubSize;
extern uint8_t sigstub[];

/* platformSignalSetup(): sets up the signal trampoline code in a user process
 * params: t - thread structure
 * returns: zero on success
 */

int platformSignalSetup(Thread *t) {
    void *trampoline = uxmalloc(sigstubSize);
    if(!trampoline) return -1;

    memcpy(trampoline, sigstub, sigstubSize);
    t->signalTrampoline = (uintptr_t) trampoline;
    return 0;
}