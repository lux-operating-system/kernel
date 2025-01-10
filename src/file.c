/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

/* Wrappers for file I/O functions */
/* None of these functions actually do anything because the microkernel has no
 * concept of files; these functions just relay the calls to lumen and request
 * a user space server to fulfill their requests */

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <kernel/file.h>
#include <kernel/io.h>
#include <kernel/sched.h>
#include <kernel/socket.h>
#include <kernel/servers.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <platform/platform.h>

int mount(Thread *t, uint64_t id, const char *src, const char *tgt, const char *type, int flags) {
    // send a request to lumen
    MountCommand *command = calloc(1, sizeof(MountCommand));
    if(!command) return -ENOMEM;

    command->header.header.command = COMMAND_MOUNT;
    command->header.header.length = sizeof(MountCommand);
    command->header.id = id;
    command->flags = flags;
    strcpy(command->source, src);
    strcpy(command->target, tgt);
    strcpy(command->type, type);

    int status = requestServer(t, 0, command);
    free(command);
    return status;
}

int lstat(Thread *t, uint64_t id, const char *path, struct stat *buffer) {
    Process *p = getProcess(t->pid);
    if(!p) return -ESRCH;

    StatCommand *command = calloc(1, sizeof(StatCommand));
    if(!command) return -ENOMEM;

    command->header.header.command = COMMAND_STAT;
    command->header.header.length = sizeof(StatCommand);
    command->header.id = id;

    if(path[0] == '/') {
        strcpy(command->path, path);
    } else {
        strcpy(command->path, p->cwd);
        if(strlen(p->cwd) > 1) command->path[strlen(command->path)] = '/';
        strcpy(command->path + strlen(command->path), path);
    }

    int status = requestServer(t, 0, command);
    free(command);
    return status;
}

int fstat(Thread *t, uint64_t id, int fd, struct stat *buffer) {
    Process *p = getProcess(t->pid);
    if(!p) return -ESRCH;
    if(fd < 0 || fd >= MAX_IO_DESCRIPTORS) return -EBADF;
    if(!p->io[fd].valid || !p->io[fd].data) return -EBADF;  // ensure valid file descriptor
    if(p->io[fd].type != IO_FILE) return -EBADF;

    FileDescriptor *file = (FileDescriptor *) p->io[fd].data;
    return lstat(t, id, file->abspath, buffer);
}

int open(Thread *t, uint64_t id, const char *path, int flags, mode_t mode) {
    OpenCommand *command = calloc(1, sizeof(OpenCommand));
    if(!command) return -ENOMEM;

    Process *p = getProcess(t->pid);
    if(!p) return -ESRCH;

    command->header.header.command = COMMAND_OPEN;
    command->header.header.length = sizeof(OpenCommand);
    command->header.id = id;
    command->flags = flags;
    command->mode = mode & (S_IRWXU | S_IRWXG | S_IRWXO);
    command->uid = p->user;
    command->gid = p->group;
    command->umask = p->umask;

    if(path[0] == '/') {
        strcpy(command->abspath, path);
    } else {
        strcpy(command->abspath, p->cwd);
        if(strlen(p->cwd) > 1) command->abspath[strlen(command->abspath)] = '/';
        strcpy(command->abspath + strlen(command->abspath), path);
    }

    int status = requestServer(t, 0, command);
    free(command);
    return status;
}

ssize_t readFile(Thread *t, uint64_t id, IODescriptor *iod, void *buffer, size_t count) {
    Process *p = getProcess(t->pid);
    if(!p) return -ESRCH;

    FileDescriptor *fd = (FileDescriptor *) iod->data;
    if(!fd) return -EBADF;

    if(!(iod->flags & O_RDONLY)) return -EPERM;

    RWCommand *command = calloc(1, sizeof(RWCommand));
    if(!command) return -ENOMEM;

    command->header.header.command = COMMAND_READ;
    command->header.header.length = sizeof(RWCommand);
    command->header.id = id;
    command->uid = p->user;
    command->gid = p->group;
    command->position = fd->position;
    command->flags = iod->flags;
    command->length = count;
    command->id = fd->id;
    strcpy(command->device, fd->device);
    strcpy(command->path, fd->path);

    int status = requestServer(t, fd->sd, command);

    free(command);
    return status;
}

ssize_t writeFile(Thread *t, uint64_t id, IODescriptor *iod, const void *buffer, size_t count) {
    Process *p = getProcess(t->pid);
    if(!p) return -ESRCH;

    FileDescriptor *fd = (FileDescriptor *) iod->data;
    if(!fd) return -EBADF;

    if(!(iod->flags & O_WRONLY)) return -EPERM;

    RWCommand *command = calloc(1, sizeof(RWCommand) + count);
    if(!command) return -ENOMEM;

    command->header.header.command = COMMAND_WRITE;
    command->header.header.length = sizeof(RWCommand) + count;
    command->header.id = id;
    command->uid = p->user;
    command->gid = p->group;

    // persistent file system drivers will know to append when the position
    // is a negative value
    if(iod->flags & O_APPEND) command->position = -1;
    else command->position = fd->position;

    command->flags = iod->flags;
    command->length = count;
    command->id = fd->id;
    strcpy(command->device, fd->device);
    strcpy(command->path, fd->path);
    memcpy(command->data, buffer, count);

    if(fd->charDev) command->silent = 1;

    int status = requestServer(t, fd->sd, command);
    free(command);
    return status;
}

int closeFile(Thread *t, int fd) {
    if(fd < 0 || fd >= MAX_IO_DESCRIPTORS) return -EBADF;

    Process *p;
    if(t) p = getProcess(t->pid);
    else p = getProcess(getKernelPID());
    if(!p) return -ESRCH;

    FileDescriptor *file = (FileDescriptor *) p->io[fd].data;
    if(!file) return -EBADF;

    // TODO: flush the file buffers here to allow drivers to implement caching

    file->refCount--;
    if(!file->refCount) free(file);

    closeIO(p, &p->io[fd]);
    return 0;
}

off_t lseek(Thread *t, int fd, off_t offset, int where) {
    if(fd < 0 || fd >= MAX_IO_DESCRIPTORS) return -EBADF;

    Process *p;
    if(t) p = getProcess(t->pid);
    else p = getProcess(getKernelPID());
    if(!p) return -ESRCH;

    FileDescriptor *file = (FileDescriptor *) p->io[fd].data;
    if(!file) return -EBADF;

    off_t newOffset;

    switch(where) {
    case SEEK_SET:
        newOffset = offset;
        break;
    case SEEK_CUR:
        newOffset = file->position + offset;
        break;
    default:
        /* TODO: SEEK_END */
        newOffset = -1;
    }

    if(newOffset < 0) return -EINVAL;
    file->position = newOffset;
    return newOffset;
}

int fcntl(Thread *t, int fd, int cmd, uintptr_t arg) {
    if(fd < 0 || fd >= MAX_IO_DESCRIPTORS) return -EBADF;
    Process *p = getProcess(t->pid);
    if(!p) return -ESRCH;

    if(!p->io[fd].valid) return -EBADF;
    FileDescriptor *file;
    SocketDescriptor *socket;

    int status = 0;
    
    switch(cmd) {
    case F_DUPFD:
    case F_DUPFD_CLOEXEC:
    case F_DUPFD_CLOFORK:
        if(((int) arg) < 0 || ((int)arg) >= MAX_IO_DESCRIPTORS)
            return -EBADF;

        IODescriptor *iod = NULL;
        int dupfd = openIO(p, (void **) &iod);
        if(dupfd < 0) return dupfd;
        if(dupfd < arg) {
            iod->valid = false;
            return -EMFILE;
        }

        iod->type = p->io[fd].type;
        iod->flags = p->io[fd].flags;
        iod->data = p->io[fd].data;

        if(iod->type == IO_FILE) {
            file = (FileDescriptor *) iod->data;
            file->refCount++;
        } else if(iod->type == IO_SOCKET) {
            socket = (SocketDescriptor *) iod->data;
            socket->refCount++;
        }

        iod->flags &= ~(FD_CLOEXEC | FD_CLOFORK);
        if(cmd == F_DUPFD_CLOEXEC) iod->flags |= FD_CLOEXEC;
        else if(cmd == F_DUPFD_CLOFORK) iod->flags |= FD_CLOFORK;

        return dupfd;

    case F_GETFD:
        if(p->io[fd].flags & O_CLOEXEC) status |= FD_CLOEXEC;
        if(p->io[fd].flags & O_CLOFORK) status |= FD_CLOFORK;
        return status;

    case F_GETFL:
        return (int) p->io[fd].flags & (O_APPEND|O_NONBLOCK|O_SYNC|O_DSYNC|O_RDONLY|O_WRONLY|O_RDWR);

    case F_SETFD:
        if(arg & FD_CLOEXEC) p->io[fd].flags |= O_CLOEXEC;
        else p->io[fd].flags &= ~(O_CLOEXEC);
        if(arg & FD_CLOFORK) p->io[fd].flags |= O_CLOFORK;
        else p->io[fd].flags &= ~(O_CLOFORK);
        break;

    case F_SETFL:
        if(arg & O_APPEND) p->io[fd].flags |= O_APPEND;
        else p->io[fd].flags &= ~(O_APPEND);
        if(arg & O_NONBLOCK) p->io[fd].flags |= O_NONBLOCK;
        else p->io[fd].flags &= ~(O_NONBLOCK);
        if(arg & O_SYNC) p->io[fd].flags |= O_SYNC;
        else p->io[fd].flags &= ~(O_SYNC);
        if(arg & O_DSYNC) p->io[fd].flags |= O_DSYNC;
        else p->io[fd].flags &= ~(O_DSYNC);
        break;
    
    case F_GETPATH:
        if(p->io[fd].type != IO_FILE)
            return -EBADF;
        file = (FileDescriptor *) p->io[fd].data;
        char *path = (char *) arg;
        strcpy(path, file->abspath);
        return strlen(path);

    default:
        return -EINVAL;
    }

    return 0;
}

mode_t umask(Thread *t, mode_t cmask) {
    Process *p = getProcess(t->pid);
    mode_t old = p->umask;
    cmask &= (S_IRWXU | S_IRWXG | S_IRWXO);
    p->umask = cmask;
    return old;
}

int chown(Thread *t, uint64_t id, const char *path, uid_t owner, gid_t group) {
    Process *p = getProcess(t->pid);
    if(!p) return -ESRCH;

    ChownCommand *command = calloc(1, sizeof(ChownCommand));
    if(!command) return -ENOMEM;

    command->header.header.command = COMMAND_CHOWN;
    command->header.header.length = sizeof(ChownCommand);
    command->header.id = id;
    command->uid = p->user;
    command->gid = p->group;
    command->newUid = owner;
    command->newGid = group;

    if(path[0] == '/') {
        strcpy(command->path, path);
    } else {
        strcpy(command->path, p->cwd);
        if(strlen(p->cwd) > 1) command->path[strlen(command->path)] = '/';
        strcpy(command->path + strlen(command->path), path);
    }

    int status = requestServer(t, 0, command);
    free(command);
    return status;
}

int chmod(Thread *t, uint64_t id, const char *path, mode_t mode) {
    Process *p = getProcess(t->pid);
    if(!p) return -ESRCH;

    ChmodCommand *command = calloc(1, sizeof(ChmodCommand));
    if(!command) return -ENOMEM;

    command->header.header.command = COMMAND_CHMOD;;
    command->header.header.length = sizeof(ChmodCommand);
    command->header.id = id;
    command->uid = p->user;
    command->gid = p->group;
    command->mode = mode & (S_IRWXU | S_IRWXG | S_IRWXO);

    if(path[0] == '/') {
        strcpy(command->path, path);
    } else {
        strcpy(command->path, p->cwd);
        if(strlen(p->cwd) > 1) command->path[strlen(command->path)] = '/';
        strcpy(command->path + strlen(command->path), path);
    }

    int status = requestServer(t, 0, command);
    free(command);
    return status;
}

int mkdir(Thread *t, uint64_t id, const char *path, mode_t mode) {
    Process *p = getProcess(t->pid);
    if(!p) return -ESRCH;

    MkdirCommand *command = calloc(1, sizeof(MkdirCommand));
    if(!command) return -ENOMEM;

    command->header.header.command = COMMAND_MKDIR;
    command->header.header.length = sizeof(MkdirCommand);
    command->header.id = id;
    command->uid = p->user;
    command->gid = p->group;
    command->umask = p->umask;
    command->mode = mode & (S_IRWXU | S_IRWXG | S_IRWXO);

    if(path[0] == '/') {
        strcpy(command->path, path);
    } else {
        strcpy(command->path, p->cwd);
        if(strlen(p->cwd) > 1) command->path[strlen(command->path)] = '/';
        strcpy(command->path + strlen(command->path), path);
    }

    int status = requestServer(t, 0, command);
    free(command);
    return status;
}

int utime(Thread *t, uint64_t id, const char *path, const struct utimbuf *times) {
    Process *p = getProcess(t->pid);
    if(!p) return -ESRCH;

    UtimeCommand *command = calloc(1, sizeof(UtimeCommand));
    if(!command) return -ENOMEM;

    command->header.header.command = COMMAND_UTIME;
    command->header.header.length = sizeof(UtimeCommand);
    command->header.id = id;
    command->uid = p->user;
    command->gid = p->group;

    if(times) {
        command->accessTime = times->actime;
        command->modifiedTime = times->modtime;
    } else {
        command->accessTime = platformTimestamp();
        command->modifiedTime = command->accessTime;
    }

    if(path[0] == '/') {
        strcpy(command->path, path);
    } else {
        strcpy(command->path, p->cwd);
        if(strlen(p->cwd) > 1) command->path[strlen(command->path)] = '/';
        strcpy(command->path + strlen(command->path), path);
    }

    int status = requestServer(t, 0, command);
    free(command);
    return status;
}

int link(Thread *t, uint64_t id, const char *old, const char *new) {
    Process *p = getProcess(t->pid);
    if(!p) return -ESRCH;

    LinkCommand *command = calloc(1, sizeof(LinkCommand));
    if(!command) return -ENOMEM;

    command->header.header.command = COMMAND_LINK;
    command->header.header.length = sizeof(LinkCommand);
    command->header.id = id;
    command->uid = p->user;
    command->gid = p->group;

    if(old[0] == '/') {
        strcpy(command->oldPath, old);
    } else {
        strcpy(command->oldPath, p->cwd);
        if(strlen(p->cwd) > 1) command->oldPath[strlen(command->oldPath)] = '/';
        strcpy(command->oldPath + strlen(command->oldPath), old);
    }

    if(new[0] == '/') {
        strcpy(command->newPath, new);
    } else {
        strcpy(command->newPath, p->cwd);
        if(strlen(p->cwd) > 1) command->newPath[strlen(command->newPath)] = '/';
        strcpy(command->newPath + strlen(command->newPath), new);
    }

    int status = requestServer(t, 0, command);
    free(command);
    return status;
}

int symlink(Thread *t, uint64_t id, const char *old, const char *new) {
    Process *p = getProcess(t->pid);
    if(!p) return -ESRCH;

    LinkCommand *command = calloc(1, sizeof(LinkCommand));
    if(!command) return -ENOMEM;

    command->header.header.command = COMMAND_SYMLINK;
    command->header.header.length = sizeof(LinkCommand);
    command->header.id = id;
    command->uid = p->user;
    command->gid = p->group;

    if(old[0] == '/') {
        strcpy(command->oldPath, old);
    } else {
        strcpy(command->oldPath, p->cwd);
        if(strlen(p->cwd) > 1) command->oldPath[strlen(command->oldPath)] = '/';
        strcpy(command->oldPath + strlen(command->oldPath), old);
    }

    if(new[0] == '/') {
        strcpy(command->newPath, new);
    } else {
        strcpy(command->newPath, p->cwd);
        if(strlen(p->cwd) > 1) command->newPath[strlen(command->newPath)] = '/';
        strcpy(command->newPath + strlen(command->newPath), new);
    }

    int status = requestServer(t, 0, command);
    free(command);
    return status;
}

int unlink(Thread *t, uint64_t id, const char *path) {
    Process *p = getProcess(t->pid);
    if(!p) return -ESRCH;

    UnlinkCommand *command = calloc(1, sizeof(UnlinkCommand));
    if(!command) return -ENOMEM;

    command->header.header.command = COMMAND_UNLINK;
    command->header.header.length = sizeof(UnlinkCommand);
    command->header.id = id;
    command->uid = p->user;
    command->gid = p->group;

    if(path[0] == '/') {
        strcpy(command->path, path);
    } else {
        strcpy(command->path, p->cwd);
        if(strlen(p->cwd) > 1) command->path[strlen(command->path)] = '/';
        strcpy(command->path + strlen(command->path), path);
    }

    int status = requestServer(t, 0, command);
    free(command);
    return status;
}

ssize_t readlink(Thread *t, uint64_t id, const char *path, char *buf, size_t bufsiz) {
    Process *p = getProcess(t->pid);
    if(!p) return -ESRCH;

    ReadLinkCommand *command = calloc(1, sizeof(ReadLinkCommand));
    if(!command) return -ENOMEM;

    command->header.header.command = COMMAND_READLINK;
    command->header.header.length = sizeof(ReadLinkCommand);
    command->header.id = id;
    command->uid = p->user;
    command->gid = p->group;

    if(path[0] == '/') {
        strcpy(command->path, path);
    } else {
        strcpy(command->path, p->cwd);
        if(strlen(p->cwd) > 1) command->path[strlen(command->path)] = '/';
        strcpy(command->path + strlen(command->path), path);
    }

    int status = requestServer(t, 0, command);
    free(command);
    return status;
}

int fsync(Thread *t, uint64_t id, int fd) {
    Process *p = getProcess(t->pid);
    if(!p) return -ESRCH;
    if(!p->io[fd].valid || (p->io[fd].type != IO_FILE)) return -EBADF;

    FileDescriptor *file = (FileDescriptor *) p->io[fd].data;
    if(!file) return -EBADF;

    FsyncCommand *cmd = calloc(1, sizeof(FsyncCommand));
    if(!cmd) return -ENOMEM;

    cmd->header.header.command = COMMAND_FSYNC;
    cmd->header.header.length = sizeof(FsyncCommand);
    cmd->header.id = id;
    cmd->close = 0;
    cmd->uid = p->user;
    cmd->gid = p->group;
    cmd->id = file->id;
    strcpy(cmd->path, file->path);
    strcpy(cmd->device, file->device);
    
    int status = requestServer(t, file->sd, cmd);
    free(cmd);
    return status;
}
