/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <platform/platform.h>
#include <platform/context.h>
#include <kernel/sched.h>
#include <kernel/logger.h>
#include <kernel/elf.h>
#include <kernel/modules.h>
#include <kernel/signal.h>

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
    strcpy(process->command, "lumen");

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
 * params: id - unique syscall ID
 * params: name - file name of the program
 * params: argv - arguments to be passed to the program
 * params: envp - environmental variables to be passed
 * returns: should not return on success
 */

int execve(Thread *t, uint16_t id, const char *name, const char **argv, const char **envp) {
    // request an external server to load the executable for us
    ExecCommand *cmd = calloc(1, sizeof(ExecCommand));
    if(!cmd) return -ENOMEM;

    Process *p = getProcess(t->pid);
    if(!p) {
        free(cmd);
        return -ESRCH;
    }

    cmd->header.header.command = COMMAND_EXEC;
    cmd->header.header.length = sizeof(ExecCommand);
    cmd->header.id = id;
    cmd->uid = p->user;
    cmd->gid = p->group;
    strcpy(cmd->path, name);

    int status = requestServer(t, 0, cmd);
    free(cmd);
    return status;
}

/* execveHandle(): handles the response for execve()
 * params: msg - response message structure
 * returns: should not return on success
 */

int execveHandle(void *msg) {
    ExecCommand *cmd = (ExecCommand *) msg;

    Thread *t = getThread(cmd->header.header.requester);
    if(!t) return -ESRCH;

    Process *p = getProcess(t->pid);
    if(!p) return -ESRCH;

    SyscallRequest *req = &t->syscall;

    // temporarily switch to the thread's context so we can parse
    // arguments and environmental variables
    threadUseContext(t->tid);
    int argc = 0, envc = 0;

    const char **argvSrc = (const char **) req->params[1];
    const char **envpSrc = (const char **) req->params[2];

    while(*argvSrc) {
        argc++;
        argvSrc++;
    }

    while(*envpSrc) {
        envc++;
        envpSrc++;
    }

    argvSrc = (const char **) req->params[1];
    envpSrc = (const char **) req->params[2];

    char **argv = calloc(argc+1, sizeof(char *));
    char **envp = calloc(envc+1, sizeof(char *));

    if(!argv || !envp) return -ENOMEM;

    memset(p->command, 0, ARG_MAX);

    for(int i = 0; argc && (i < argc); i++) {
        argv[i] = malloc(strlen(argvSrc[i]) + 1);
        if(!argv[i]) return -ENOMEM;

        strcpy(argv[i], argvSrc[i]);
        if(i == 0) {
            strcpy(p->name, argvSrc[0]);
            strcpy(p->command, argvSrc[0]);
        } else {
            strcpy(p->command + strlen(p->command), " ");
            strcpy(p->command + strlen(p->command), argvSrc[i]);
        }
    }

    for(int i = 0; envc && (i < envc); i++) {
        envp[i] = malloc(strlen(envpSrc[i]) + 1);
        if(!envp[i]) return -ENOMEM;

        strcpy(envp[i], envpSrc[i]);
    }

    // null terminate the args and env in accordance with posix
    argv[argc] = NULL;
    envp[envc] = NULL;

    int status = execmve(t, cmd->elf, (const char **) argv, (const char **) envp);

    // now free the memory we used up for parsing the args
    for(int i = 0; argc && (i < argc); i++) {
        if(argv[i]) free(argv[i]);
    }

    for(int i = 0; envc && (i < envc); i++) {
        if(envp[i]) free(envp[i]);
    }

    free(argv);
    free(envp);
    return status;
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

    Process *p = getProcess(t->pid);
    if(!p) {
        schedRelease();
        return -ESRCH;
    }

    // set new name
    strcpy(p->name, name);
    strcpy(p->command, name);

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

    int status = execmve(t, image, argv, NULL);
    free(image);
    schedRelease();
    return status;
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
    uint64_t oldHighest = t->highest;

    void *newctx = calloc(1, PLATFORM_CONTEXT_SIZE);
    if(!newctx) {
        return -1;
    }

    if(!platformCreateContext(newctx, PLATFORM_CONTEXT_USER, 0, 0)) {
        free(newctx);
        return -1;
    }

    void *oldctx = t->context;
    t->context = newctx;

    threadUseContext(t->tid);   // switch to our new context

    // parse the binary
    uint64_t highest;
    uint64_t entry = loadELF(image, &highest);
    if(!entry || !highest) {
        t->context = oldctx;
        free(newctx);
        schedRelease();
        return -1;
    }

    if(platformSetContext(t, entry, highest, argv, envp)) {
        t->context = oldctx;
        free(newctx);
        return -1;
    }

    // close file/socket descriptors marked as O_CLOEXEC
    // this fixes a security risk i realized too late
    Process *p = getProcess(t->tid);
    p->umask = 0;
    for(int i = 0; i < MAX_IO_DESCRIPTORS; i++) {
        if(p->io[i].valid && (p->io[i].flags & O_CLOEXEC)) {
            p->io[i].valid = false;
            p->io[i].data = NULL;
            p->io[i].type = 0;
            p->io[i].flags = 0;

            p->iodCount--;
        }
    }

    // set up default signal handlers
    t->signals = signalDefaults();

    // TODO: here we've successfully loaded the new program, but we also need
    // to free up memory used by the original program
    platformCleanThread(oldctx, oldHighest);
    free(oldctx);

    t->status = THREAD_QUEUED;
    schedAdjustTimeslice();
    return 0; // return to syscall dispatcher; the thread will not see this return
}