/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

#include <kernel/dirent.h>
#include <kernel/servers.h>
#include <kernel/io.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/* Just as with the file system calls, these are just wrappers and do not
 * implement any actual functionality at the kernel level */

int opendir(Thread *t, uint64_t id, const char *path) {
    if(strlen(path) >= MAX_FILE_PATH) return -ENAMETOOLONG;
    Process *p = getProcess(t->pid);
    if(!p) return -ESRCH;

    OpendirCommand *cmd = calloc(1, sizeof(OpendirCommand));
    if(!cmd) return -ENOMEM;

    cmd->header.header.command = COMMAND_OPENDIR;
    cmd->header.header.length = sizeof(OpendirCommand);
    cmd->header.id = id;
    cmd->uid = p->user;
    cmd->gid = p->group;

    if(path[0] == '/') {
        strcpy(cmd->abspath, path);
    } else {
        strcpy(cmd->abspath, p->cwd);
        cmd->abspath[strlen(cmd->abspath)] = '/';
        strcpy(cmd->abspath + strlen(cmd->abspath), path);
    }
    
    int status = requestServer(t, cmd);
    free(cmd);
    return status;
}

int readdir_r(Thread *t, uint64_t id, DIR *dir, struct dirent *entry, struct dirent **result) {
    Process *p = getProcess(t->pid);
    if(!p) return -ESRCH;

    // input validation
    int dd = (int) dir & ~(DIRECTORY_DESCRIPTOR_FLAG);
    if(dd < 0 || dd >= MAX_IO_DESCRIPTORS) return -EBADF;
    if(!p->io[dd].valid || p->io[dd].type != IO_DIRECTORY) return -EBADF;

    DirectoryDescriptor *descriptor = (DirectoryDescriptor *) p->io[dd].data;
    if(!descriptor) return -EBADF;

    ReaddirCommand *cmd = calloc(1, sizeof(ReaddirCommand));
    if(!cmd) return -ENOMEM;

    cmd->header.header.command = COMMAND_READDIR;
    cmd->header.header.length = sizeof(ReaddirCommand);
    cmd->header.id = id;
    cmd->position = descriptor->position;
    strcpy(cmd->path, descriptor->path);
    strcpy(cmd->device, descriptor->device);

    int status = requestServer(t, cmd);
    free(cmd);
    return status;
}

void seekdir(Thread *t, DIR *dir, long position) {
    Process *p = getProcess(t->pid);
    if(!p) return;

    int dd = (int) dir & ~(DIRECTORY_DESCRIPTOR_FLAG);
    if(dd < 0 || dd >= MAX_IO_DESCRIPTORS) return;
    if(!p->io[dd].valid || p->io[dd].type != IO_DIRECTORY) return;

    DirectoryDescriptor *descriptor = (DirectoryDescriptor *) p->io[dd].data;
    if(!descriptor) return;

    descriptor->position = position;
    return;
}

long telldir(Thread *t, DIR *dir) {
    Process *p = getProcess(t->pid);
    if(!p) return -ESRCH;

    int dd = (int) dir & ~(DIRECTORY_DESCRIPTOR_FLAG);
    if(dd < 0 || dd >= MAX_IO_DESCRIPTORS) return -EBADF;
    if(!p->io[dd].valid || p->io[dd].type != IO_DIRECTORY) return -EBADF;

    DirectoryDescriptor *descriptor = (DirectoryDescriptor *) p->io[dd].data;
    if(!descriptor) return -EBADF;

    return descriptor->position;
}

int closedir(Thread *t, DIR *dir) {
    Process *p = getProcess(t->pid);
    if(!p) return -ESRCH;

    int dd = (int) dir & ~(DIRECTORY_DESCRIPTOR_FLAG);
    if(dd < 0 || dd >= MAX_IO_DESCRIPTORS) return -EBADF;
    if(!p->io[dd].valid || p->io[dd].type != IO_DIRECTORY) return -EBADF;

    DirectoryDescriptor *descriptor = (DirectoryDescriptor *) p->io[dd].data;
    if(!descriptor) return -EBADF;

    // destroy the descriptor
    if(!p->io[dd].clone)
        free(p->io[dd].data);

    p->io[dd].valid = false;
    p->io[dd].type = 0;
    p->io[dd].flags = 0;
    p->io[dd].data = NULL;
    p->io[dd].clone = false;
    p->iodCount--;

    return 0;
}
