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
#include <kernel/modules.h>

int execmve(Thread *, void *, const char **, const char **);

/* execveMemory(): executes a program from memory
 * params: ptr - pointer to the program in memory
 * params: argv - arguments to be passed to the program
 * params: envp - environmental variables to be passed
 * returns: PID, zero on fail
 */

pid_t execveMemory(const void *ptr, const char **argv, const char **envp) {
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
    if(!process->threads) {
        free(process);
        schedRelease();
        return 0;
    }

    process->threads[0] = calloc(1, sizeof(Thread));
    if(!process->threads[0]) {
        free(process->threads);
        free(process);
        schedRelease();
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
        return 0;
    }

    if(!platformCreateContext(process->threads[0]->context, PLATFORM_CONTEXT_USER, 0, 0)) {
        free(process->threads[0]->context);
        free(process->threads[0]);
        free(process->threads);
        free(process);
        schedRelease();
        return 0;
    }

    threadUseContext(pid);

    uint64_t highest;
    uint64_t entry = loadELF(ptr, &highest);

    if(platformSetContext(process->threads[0], entry, highest, argv, envp)) {
        threadUseContext(getTid());
        free(process->threads[0]->context);
        free(process->threads[0]);
        free(process->threads);
        free(process);
        schedRelease();
        return 0;
    }

    process->pages = process->threads[0]->pages;

    KDEBUG("created new process with pid %d\n", pid);

    processes++;
    threads++;
    //schedAdjustTimeslice();

    threadUseContext(getTid());
    schedRelease();
    return pid;
}

/* execve(): replaces the current program, executes a program from a file
 * params: t - parent thread structure
 * params: name - file name of the program
 * params: argv - arguments to be passed to the program
 * params: envp - environmental variables to be passed
 * returns: should not return on success
 */

int execve(Thread *t, const char *name, const char **argv, const char **envp) {
    return 0;    /* todo */
}

/* execrdv(): replaces the current program, executes a program from the ramdisk
 * this is only used before file system drivers are loaded
 * params: t - parent thread structure
 * params: name - file name of the program
 * params: argv - arguments to be passed to the program
 * returns: should not return on success
 */

int execrdv(Thread *t, const char *name, const char **argv) {
    schedLock();

    // load from ramdisk
    int64_t size = ramdiskFileSize(name);
    if(size <= sizeof(ELFFileHeader)) {
        schedRelease();
        return -1;
    }

    void *image = malloc(size);
    if(!image) {
        schedRelease();
        return -1;
    }

    if(ramdiskRead(image, name, size) != size) {
        schedRelease();
        return -1;
    }

    return execmve(t, image, argv, NULL);
}

/* execmve(): helper function that replaces the current running program from memory
 * params: t - parent thread structure
 * params: image - image of the program in memory
 * params: argv - arguments to be passed to the program
 * params: envp - environmental variables
 * returns: should not return on success
 */

int execmve(Thread *t, void *image, const char **argv, const char **envp) {
    // create the new context before deleting the current one
    // this guarantees we can return on failure
    void *newctx = calloc(1, PLATFORM_CONTEXT_SIZE);
    if(!newctx) {
        free(image);
        schedRelease();
        return -1;
    }

    if(!platformCreateContext(newctx, PLATFORM_CONTEXT_USER, 0, 0)) {
        free(newctx);
        free(image);
        schedRelease();
        return -1;
    }

    void *oldctx = t->context;
    t->context = newctx;

    threadUseContext(t->tid);   // switch to our new context

    // parse the binary
    uint64_t highest;
    uint64_t entry = loadELF(image, &highest);
    free(image);
    if(!entry || !highest) {
        t->context = oldctx;
        free(newctx);
        schedRelease();
        return -1;
    }

    if(platformSetContext(t, entry, highest, argv, envp)) {
        t->context = oldctx;
        free(newctx);
        schedRelease();
        return -1;
    }

    // close file/socket descriptors marked as O_CLOEXEC
    // this fixes a security risk i realized too late
    Process *p = getProcess(t->tid);
    for(int i = 0; i < MAX_IO_DESCRIPTORS; i++) {
        if(p->io[i].valid && (p->io[i].flags & O_CLOEXEC)) {
            p->io[i].valid = false;
            p->io[i].data = NULL;
            p->io[i].type = 0;
            p->io[i].flags = 0;

            p->iodCount--;
        }
    }

    // TODO: here we've successfully loaded the new program, but we also need
    // to free up memory used by the original program
    free(oldctx);

    t->status = THREAD_QUEUED;
    schedAdjustTimeslice();
    schedRelease();
    return 0; // return to syscall dispatcher; the thread will not see this return
}