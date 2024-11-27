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

    void *siginfo = umalloc(sizeof(siginfo_t));
    if(!siginfo) {
        free(trampoline);
        return -1;
    }

    void *sigctx = umalloc(PLATFORM_CONTEXT_SIZE);
    if(!sigctx) {
        free(trampoline);
        free(siginfo);
        return -1;
    }

    memcpy(trampoline, sigstub, sigstubSize);
    t->signalTrampoline = (uintptr_t) trampoline;
    t->siginfo = (uintptr_t) siginfo;
    t->signalUserContext = (uintptr_t) sigctx;
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

    ThreadContext *ctx = (ThreadContext *) dest->context;
    platformUseContext(ctx);

    Process *p = NULL;
    if(sender) p = getProcess(sender->pid);

    siginfo_t *siginfo = (siginfo_t *) dest->siginfo;
    siginfo->si_signo = signum;
    if(sender) siginfo->si_pid = sender->tid;
    else siginfo->si_pid = 0;   // we will use pid 0 for the kernel

    if(p) siginfo->si_uid = p->user;
    else siginfo->si_uid = 0;

    siginfo->si_code = 0;       // TODO

    ThreadContext *uctx = (ThreadContext *) dest->signalUserContext;
    memcpy(uctx, dest->context, PLATFORM_CONTEXT_SIZE);

    // signal entry point
    // func(int sig, siginfo_t *info, void *ctx)
    // https://pubs.opengroup.org/onlinepubs/007904875/functions/sigaction.html

    ctx->regs.rip = handler;
    ctx->regs.rdi = signum;     // int sig
    ctx->regs.rsi = (uint64_t) siginfo;     // siginfo_t *info
    ctx->regs.rdx = (uint64_t) uctx;        // void *ctx

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