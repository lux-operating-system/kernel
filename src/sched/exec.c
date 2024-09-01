/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

#include <stdlib.h>
#include <platform/platform.h>
#include <platform/context.h>
#include <platform/x86_64.h>
#include <kernel/sched.h>
#include <kernel/logger.h>
#include <kernel/elf.h>

/* execveMemory(): executes a program from memory
 * params: ptr - pointer to the program in memory
 * params: argv - arguments to be passed to the program
 * params: envp - environmental variables to be passed
 * returns: should not return on success
 */

int execveMemory(const void *ptr, const char **argv, const char **envp) {
    setScheduling(false);
    schedLock();

    pid_t pid = processCreate();
    if(!pid) {
        schedRelease();
        return 0;
    }

    Process *process = getProcess(pid);

    // this is a blank process, so we need to create a thread for it
    process->threadCount = 1;
    process->threads = calloc(process->threadCount, sizeof(Thread *));
    if(!process->threadCount) {
        free(process);
        schedRelease();
        setScheduling(true);
        return 0;
    }

    process->threads[0] = calloc(1, sizeof(Thread));
    if(!process->threads[0]) {
        free(process->threads);
        free(process);
        schedRelease();
        setScheduling(true);
        return 0;
    }

    process->threads[0]->status = THREAD_QUEUED;
    process->threads[0]->next = NULL;
    process->threads[0]->pid = pid;
    process->threads[0]->tid = pid;
    //process->threads[0]->time = PLATFORM_TIMER_FREQUENCY;
    process->threads[0]->context = calloc(1, PLATFORM_CONTEXT_SIZE);
    if(!process->threads[0]->context) {
        free(process->threads[0]);
        free(process->threads);
        free(process);
        schedRelease();
        setScheduling(true);
        return 0;
    }

    if(!platformCreateContext(process->threads[0]->context, PLATFORM_CONTEXT_USER, 0, 0)) {
        free(process->threads[0]->context);
        free(process->threads[0]);
        free(process->threads);
        free(process);
        schedRelease();
        setScheduling(true);
        return 0;
    }

    threadUseContext(pid);

    uint64_t highest;
    uint64_t entry = loadELF(ptr, &highest);

    platformSetContext(process->threads[0], entry, highest, argv, envp);

    KDEBUG("created new process with pid %d\n", pid);

    processes++;
    threads++;
    schedAdjustTimeslice();

    threadUseContext(getTid());
    schedRelease();
    setScheduling(true);
    return 0;
}

/* execve(): executes a program from a file
 * params: name - file name of the program
 * params: argv - arguments to be passed to the program
 * params: envp - environmental variables to be passed
 * returns: should not return on success
 */

int execve(const char *name, const char **argv, const char **envp) {
    return 0;    /* todo */
}
