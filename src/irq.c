/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <kernel/irq.h>
#include <kernel/sched.h>
#include <kernel/logger.h>
#include <platform/platform.h>
#include <platform/lock.h>

static lock_t lock = LOCK_INITIAL;
static IRQ *irqs = NULL;

/* installIRQ(): installs an IRQ handler
 * params: t - calling thread
 * params: pin - IRQ number, platform-dependent
 * params: h - IRQ handler structure
 * returns: interrupt pin on success, negative error code on fail
 */

int installIRQ(Thread *t, int pin, IRQHandler *h) {
    if(!t) t = getKernelThread();
    Process *p = getProcess(t->pid);
    if(!p) return -ESRCH;
    if(p->user) return -EPERM;      // only root can do this

    if(pin < 0 || pin > platformGetMaxIRQ()) return -EIO;

    if(!irqs) {
        irqs = calloc(platformGetMaxIRQ(), sizeof(IRQ));
        if(!irqs) return -ENOMEM;

        for(int i = 0; i < platformGetMaxIRQ(); i++) irqs[i].pin = i;
    }

    // install the new handler
    acquireLockBlocking(&lock);

    int actual = platformConfigureIRQ(t, pin, h);   // allow the platform to redirect IRQs
    if(actual < 0) {
        releaseLock(&lock);
        return actual;
    }

    IRQHandler *newHandlers = realloc(irqs[pin].handlers, (irqs[pin].devices+1)*sizeof(IRQHandler));
    if(!newHandlers) {
        releaseLock(&lock);
        return -ENOMEM;
    }

    irqs[pin].handlers = newHandlers;
    memcpy(&newHandlers[irqs[pin].devices], h, sizeof(IRQHandler));
    irqs[pin].devices++;
    
    if(actual == pin) KDEBUG("device '%s' is using IRQ %d\n", h->name, actual);
    else KDEBUG("device '%s' is using IRQ %d (redirected from IRQ %d)\n", h->name, actual, pin);

    releaseLock(&lock);
    return actual;
}

/* dispatchIRQ(): dispatches an IRQ to its handler
 * params: pin - IRQ number, platform-dependent
 * returns: nothing
 */

void dispatchIRQ(uint64_t pin) {
    // TODO
    platformAcknowledgeIRQ(NULL);   // just send EOI for now
}