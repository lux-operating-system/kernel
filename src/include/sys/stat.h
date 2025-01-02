/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

#pragma once

#include <sys/types.h>

struct stat {
    dev_t st_dev;
    ino_t st_ino;
    mode_t st_mode;
    nlink_t st_nlink;
    uid_t st_uid;
    gid_t st_gid;
    dev_t st_rdev;
    off_t st_size;
    time_t st_atime, st_mtime, st_ctime;
    blksize_t st_blksize;
    blkcnt_t st_blocks;
};

#define S_IFBLK             0x00000001      // block special
#define S_IFCHR             0x00000002      // character special
#define S_IFIFO             0x00000003      // named pipe
#define S_IFREG             0x00000004      // regular file
#define S_IFDIR             0x00000005      // directory
#define S_IFLNK             0x00000006      // symbolic link
#define S_IFSOCK            0x00000007      // socket
#define S_IFMT              0x00000007      // mask for file type

#define S_IRUSR             0x00000008      // owner read perms
#define S_IWUSR             0x00000010      // owner write perms
#define S_IXUSR             0x00000020      // owner execute perms
#define S_IRWXU             (S_IRUSR | S_IWUSR | S_IXUSR)

#define S_IRGRP             0x00000040      // group read perms
#define S_IWGRP             0x00000080      // group write perms
#define S_IXGRP             0x00000100      // group execute perms
#define S_IRWXG             (S_IRGRP | S_IWGRP | S_IXGRP)

#define S_IROTH             0x00000200      // others read perms
#define S_IWOTH             0x00000400      // others write perms
#define S_IXOTH             0x00000800      // others execute perms
#define S_IRWXO             (S_IROTH | S_IWOTH | S_IXOTH)

#define S_ISUID             0x00001000      // set UID when executed
#define S_ISGID             0x00002000      // set GID when execute
#define S_ISVTX             0x00004000      // prevent directory deletion

#define S_ISBLK(m)          ((m & S_IFMT) == S_IFBLK)
#define S_ISCHR(m)          ((m & S_IFMT) == S_IFCHR)
#define S_ISDIR(m)          ((m & S_IFMT) == S_IFDIR)
#define S_ISFIFO(m)         ((m & S_IFMT) == S_IFIFO)
#define S_ISREG(m)          ((m & S_IFMT) == S_IFREG)
#define S_ISLNK(m)          ((m & S_IFMT) == S_IFLNK)
#define S_ISSOCK(m)         ((m & S_IFMT) == S_IFSOCK)

int stat(Thread *, uint64_t, const char *, struct stat *);
int fstat(Thread *t, uint64_t, int, struct stat *);
