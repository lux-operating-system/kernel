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
#include <kernel/servers.h>
#include <kernel/socket.h>
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

    // only the kernel can install kernel-level IRQ handlers
    if(h->kernel && t != getKernelThread()) return -EPERM;

    if(pin < 0 || pin > platformGetMaxIRQ()) return -EIO;

    if(!irqs) {
        irqs = calloc(platformGetMaxIRQ() + 1, sizeof(IRQ));
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
    
    if(actual == pin) KDEBUG("device '%s' is using IRQ %d, currently used by %d device%s\n", h->name, actual, irqs[pin].devices, irqs[pin].devices != 1 ? "s" : "");
    else KDEBUG("device '%s' is using IRQ %d (redirected from IRQ %d), currently used by %d device%s\n", h->name, actual, pin, irqs[pin].devices, irqs[pin].devices != 1 ? "s" : "");

    releaseLock(&lock);
    return actual;
}

/* dispatchIRQ(): dispatches an IRQ to its handler
 * params: pin - IRQ number, platform-dependent
 * returns: nothing
 */

void dispatchIRQ(uint64_t pin) {
    if(!irqs[pin].devices) {
        KWARN("IRQ %d fired but no devices are using it\n", pin);
        platformAcknowledgeIRQ(NULL);
        return;
    }

    // dispatch the interrupt to the appropriate servers (possible shared IRQs)
    Thread *k = getKernelThread();  // for socket context
    IRQCommand *irqcmd = platformGetIRQCommand();
    irqcmd->pin = pin;

    for(int i = 0; i < irqs[pin].devices; i++) {
        // notify all driver sharing this IRQ
        int sd = serverSocket(irqs[pin].handlers[i].driver);
        if(sd > 0) send(k, sd, irqcmd, sizeof(IRQCommand), 0);
    }

    platformAcknowledgeIRQ(NULL);   // end of interrupt
}
