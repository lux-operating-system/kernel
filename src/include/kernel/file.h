/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <kernel/sched.h>
#include <kernel/io.h>
#include <sys/types.h>
#include <sys/stat.h>

/* system-wide limits */
#define MAX_FILE                (1 << 18)   // 262k
#define MAX_FILE_PATH           2048

#define SEEK_SET                1
#define SEEK_CUR                2
#define SEEK_END                3

/* fcntl() commands */
#define F_DUPFD                 1
#define F_GETFD                 2
#define F_SETFD                 3
#define F_GETFL                 4
#define F_SETFL                 5
#define F_GETLK                 6
#define F_SETLK                 7
#define F_SETLKW                8
#define F_GETOWN                9
#define F_SETOWN                10
#define F_DUPFD_CLOEXEC         11
#define F_DUPFD_CLOFORK         12
#define F_GETPATH               0x8000

/* fcntl() flags */
#define FD_CLOEXEC              (O_CLOEXEC)
#define FD_CLOFORK              (O_CLOFORK)

/* file locks */
#define F_UNLOCK                1
#define F_RDLCK                 2
#define F_WRLCK                 4

/* file-specific I/O descriptor (see io.h) */
typedef struct {
    Process *process;
    char abspath[MAX_FILE_PATH];    // absolute path
    char device[MAX_FILE_PATH];     // device
    char path[MAX_FILE_PATH];       // path relative to device mountpount
    off_t position;
    uint64_t id;                    // unique ID, this is for device files
    int charDev;                    // character device boolean
    int refCount;
    int sd;                         // socket descriptor of relevant driver
} FileDescriptor;

/* file lock structure */
struct flock {
    short l_type, l_whence;
    off_t l_start, l_len;
    pid_t l_pid;
};

/* utime */
struct utimbuf {
    time_t actime;
    time_t modtime;
};

/* file system syscalls */
int open(Thread *, uint64_t, const char *, int, mode_t);
int close(Thread *, int);
ssize_t read(Thread *, uint64_t, int, void *, size_t);
ssize_t write(Thread *, uint64_t, int, const void *, size_t);
off_t lseek(Thread *, int, off_t, int);
int mount(Thread *, uint64_t, const char *, const char *, const char *, int);
int fcntl(Thread *, int, int, uintptr_t);
mode_t umask(Thread *, mode_t);
int chown(Thread *, uint64_t, const char *, uid_t, gid_t);
int chmod(Thread *, uint64_t, const char *, mode_t);
int mkdir(Thread *, uint64_t, const char *, mode_t);
int utime(Thread *, uint64_t, const char *, const struct utimbuf *);
int fsync(Thread *, uint64_t, int);

ssize_t readFile(Thread *, uint64_t, IODescriptor *, void *, size_t);
ssize_t writeFile(Thread *, uint64_t, IODescriptor *, const void *, size_t);
int closeFile(Thread *, int);

int chdir(Thread *, uint16_t, const char *);
char *getcwd(Thread *, char *, size_t);

int link(Thread *, uint64_t, const char *, const char *);
int unlink(Thread *, uint64_t, const char *);
int symlink(Thread *, uint64_t, const char *, const char *);
ssize_t readlink(Thread *, uint64_t, const char *, char *, size_t);
