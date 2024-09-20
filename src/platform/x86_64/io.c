/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Platform-Specific Code for x86_64
 */

#include <platform/context.h>
#include <kernel/sched.h>
#include <errno.h>

/* platformIoperm(): sets the I/O permissions for the current thread
 * params: t - calling thread
 * params: from - base I/O port
 * params: count - number of I/O ports to change permissions
 * params: enable - 0 to disable access, 1 to enable
 * returns: zero on success, negative error code on fail
 */

int platformIoperm(Thread *t, uintptr_t from, uintptr_t count, int enable) {
    // privilege checks were already performed in the generic ioperm()
    if((from+count-1) > 0xFFFF) return -EINVAL;     // 65536 I/O ports on x86

    ThreadContext *ctx = (ThreadContext *) t->context;

    for(int i = 0; i < count; i++) {
        int byte = (from + i) / 8;
        int bit = (from + i) % 8;

        if(enable) ctx->ioports[byte] &= ~(1 << bit);
        else ctx->ioports[byte] |= (1 << bit);
    }

    // new permissions will be enforced in the next context switch, so return
    return 0;
}