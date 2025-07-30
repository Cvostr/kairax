#ifndef _ERRNO_H
#define _ERRNO_H

#define __set_errno(x) long ret = x; if (ret < 0) { errno = -ret; ret = -1; } return ret

int* __errno_location();

#define errno (*(__errno_location()))

#define ERROR_BAD_FD                9
#define ERROR_NO_FILE               2
#define ERROR_IS_DIRECTORY          21
#define ERROR_INVALID_VALUE         22
#define ERROR_NOT_A_DIRECTORY       20        
#define ERROR_TOO_MANY_OPEN_FILES   24

#define	EPERM		                1
#define	ENOENT                      2
#define	ESRCH		                3
#define EINTR		                4
#define EIO		                    5
#define	E2BIG                       7
#define ENOEXEC                     8
#define	EBADF		                9
#define	ECHILD		                10
#define	EAGAIN                      11
#define	ENOMEM                      12
#define	EACCES		                13
#define	EBUSY                       16
#define EEXIST                      17
#define EXDEV                       18
#define	ENOTDIR		                20
#define EISDIR                      21
#define EINVAL                      22
#define	EMFILE		                24
#define ENOSPC                      28
#define	ESPIPE		                29
#define EROFS		                30
#define EPIPE		                32
#define ERANGE                      34
#define	ENOSYS		                38
#define ENOTEMPTY                   39
#define ENOTSOCK                    88
#define EADDRINUSE	                98
#define EADDRNOTAVAIL	            99
#define ECONNRESET	                104
#define EISCONN                     106
#define ENOTCONN                    107
#define ECONNREFUSED	            111
#define EHOSTUNREACH	            113
#define ETIMEDOUT                   116


#endif