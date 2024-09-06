/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

#pragma once

/* System Error Numbers */

#define E2BIG               1   // too many args
#define EACCES              2   // access denied
#define EADDRINUSE          3   // address in use
#define EADDRNOTAVAIL       4   // address not available
#define EAFNOSUPPORT        5   // address family not supported
#define EAGAIN              6   // resource busy, try again
#define EALREADY            7   // connection already in progress
#define EBADF               8   // bad file descriptor
#define EBADMSG             9   // bad message
#define EBUSY               10  // resource busy
#define ECANCELED           11  // canceled
#define ECHILD              12  // no child processes
#define ECONNABORTED        13  // connection aborted
#define ECONNREFUSED        14  // connection refused
#define ECONNRESET          15  // connection reset
#define EDEADLK             16  // deadlock
#define EDESTADDRREQ        17  // destination address required
#define EDOM                18  // function argument not in domain
#define EDQUOT              19  // reserved
#define EEXIST              20  // file exists
#define EFAULT              21  // bad address
#define EFBIG               22  // file too big
#define EHOSTUNREACH        23  // host unreachable
#define EIDRM               24  // identifier removed
#define EILSEQ              25  // illegal byte sequence
#define EINPROGRESS         26  // operation in progress
#define EINTR               27  // interrupted function
#define EINVAL              28  // invalid argument
#define EIO                 29  // I/O error
#define EISCONN             30  // socket already connected
#define EISDIR              31  // directory
#define ELOOP               32  // looping symlinks
#define EMFILE              33  // file descriptor too big
#define EMLINK              34  // too many links
#define EMSGSIZE            35  // message too large
#define EMULTIHOP           36  // reserved
#define ENAMETOOLONG        37  // file name too long
#define ENETDOWN            38  // network is down
#define ENETRESET           39  // connection aborted by network
#define ENETUNREACH         40  // network unreachable
#define ENFILE              41  // too many files open
#define ENOBUFS             42  // no buffers available
#define ENODEV              43  // no such device
#define ENOENT              44  // no such file/directory
#define ENOEXEC             45  // executable file error
#define ENOLCK              46  // no locks available
#define ENOLINK             47  // reserved
#define ENOMEM              48  // ran out of memory
#define ENOMSG              49  // no message of desired type
#define ENOPROTOOPT         50  // protocol not implemented
#define ENOSPC              51  // ran out of storage
#define ENOSYS              52  // function not supported
#define ENOCONN             53  // socket not connected
#define ENOTDIR             54  // not a directory nor symlink to directory
#define ENOTEMPTY           55  // directory not empty
#define ENOTRECOVERABLE     56  // unrecoverable error
#define ENOTSOCK            57  // not a socket
#define ENOTSUP             58  // operation not supported
#define EOPNOTSUPP          ENOTSUP
#define ENOTTY              59  // inappropriate I/O
#define ENXIO               60  // no such device or address
#define EOVERFLOW           61  // value larger than data type
#define EOWNERDEAD          62  // previous owner died
#define EPERM               63  // operation not permitted
#define EPIPE               64  // broken pipe
#define EPROTO              65  // protocol error
#define EPROTONOSUPPORT     66  // protocol not supported
#define EPROTOTYPE          67  // wrong protocol type
#define ERANGE              68  // result too large
#define EROFS               69  // read-only file system
#define ESPIPE              70  // invalid seek
#define ESRCH               71  // no such process
#define ESTALE              72  // reserved
#define ETIMEDOUT           73  // connection timeout
#define ETXTBSY             74  // text file busy
#define EWOULDBLOCK         75  // blocking operation
#define EXDEV               76  // cross-device link
