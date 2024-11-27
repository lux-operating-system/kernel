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

/* platformSendSignal(): dispatches a signal to a thread
 * params: sender - thread sending the signal
 * params: dest - thread receiving the signal
 * params: signum - signal number
 * params: handler - function to dispatch
 * returns: zero on success
 */

int platformSendSignal(Thread *sender, Thread *dest, int signum, uintptr_t handler) {
    memcpy(dest->signalContext, dest->context, PLATFORM_CONTEXT_SIZE);

    /* TODO: enter the function as func(int sig, siginfo_t *info, void *ctx)
     * instead of just func(int sig)
     * https://pubs.opengroup.org/onlinepubs/007904875/functions/sigaction.html
     */

    ThreadContext *ctx = (ThreadContext *) dest->context;
    platformUseContext(ctx);

    ctx->regs.rip = handler;
    ctx->regs.rdi = signum;
    ctx->regs.rflags = 0x202;
    ctx->regs.rsp -= 128;       // downwards of the red zone

    // ensure stack is 16-byte aligned on entry
    while(ctx->regs.rsp & 0x0F)
        ctx->regs.rsp--;
    ctx->regs.rbp = ctx->regs.rsp;

    // inject signal trampoline
    uint64_t *stack = (uint64_t *) ctx->regs.rsp;
    *stack = dest->signalTrampoline;

    return 0;
}

/* platformSigreturn(): restores context before a signal handler was invoked
 * params: t - thread to restore
 * returns: nothing
 */

void platformSigreturn(Thread *t) {
    memcpy(t->context, t->signalContext, PLATFORM_CONTEXT_SIZE);
}