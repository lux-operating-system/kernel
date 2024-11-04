/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

#include <string.h>
#include <stdlib.h>
#include <kernel/sched.h>
#include <platform/platform.h>

/* threadCleanup(): frees all memory associated with a thread and removes it
 * from the run queues
 * params: t - thread structure
 * returns: nothing
 */

void threadCleanup(Thread *t) {
    platformCleanThread(t->context, t->highest);

    Process *pc = getProcess(t->pid);
    Process *pq = getProcessQueue();
    Thread *tq;

    while(pq) {
        if(!pq->threadCount || !pq->threads) {
            pq = pq->next;
            continue;
        }

        for(int i = 0; i < pq->threadCount; i++) {
            if(pq->threads[i] == t) {
                if(i != pq->threadCount-1)
                    memmove(&pq->threads[i], &pq->threads[i+1], (pq->threadCount-i) * sizeof(Thread *));
                else
                    pq->threads[i] = NULL;

                pq->threadCount--;
                break;
            } else {
                // remove from the next queues as well
                tq = pq->threads[i];
                if(tq->next == t) tq->next = t->next;
            }
        }

        pq = pq->next;
    }

    // now remove the process that contains this thread if necessary
    if(!pc->threadCount) {
        pq = getProcessQueue();

        while(pq) {
            if(pq->next == pc) pq->next = pc->next;
            pq = pq->next;
        }

        pq = getProcess(pc->parent);
        for(int i = 0; i < pq->childrenCount; i++) {
            if(pq->children[i] == pc) {
                if(i != pq->childrenCount-1)
                    memmove(&pq->children[i], &pq->children[i+1], (pq->childrenCount-i) * sizeof(Process *));
                else
                    pq->children[i] = NULL;
                
                pq->childrenCount--;
                break;
            }
        }

        free(pc->children);
        free(pc->threads);
        free(pc);
    }

    free(t);
}