/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

#include <kernel/sched.h>
#include <kernel/file.h>
#include <kernel/servers.h>
#include <string.h>
#include <errno.h>

/* chdir(): changes the working directory of the running process
 * params: t - running thread
 * params: id - syscall ID
 * params: path - new working directory
 * returns: zero on success
 */

int chdir(Thread *t, uint16_t id, const char *path) {
    if(!path) return -EINVAL;
    if(strlen(path) > MAX_FILE_PATH) return -ENAMETOOLONG;

    Process *p = getProcess(t->pid);
    if(!p) return -ESRCH;

    ChdirCommand cmd;
    memset(&cmd, 0, sizeof(ChdirCommand));
    cmd.header.header.command = COMMAND_CHDIR;
    cmd.header.header.length = sizeof(ChdirCommand);
    cmd.header.id = id;
    cmd.uid = p->user;
    cmd.gid = p->group;

    if(path[0] == '/') {
        strcpy(cmd.path, path);
    } else {
        strcpy(cmd.path, p->cwd);
        cmd.path[strlen(cmd.path)] = '/';
        strcpy(cmd.path + strlen(cmd.path), path);
    }

    return requestServer(t, &cmd);
}