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
#define	E2BIG                       7
#define ENOEXEC                     8
#define	EBADF		                9
#define	ECHILD		                10
#define	EAGAIN                      11
#define	ENOMEM                      12
#define	EACCES		                13
#define	EBUSY                       16
#define EEXIST                      17
#define	ENOTDIR		                20
#define EISDIR                      21
#define EINVAL                      22
#define	EMFILE		                24
#define	ESPIPE		                29
#define ENOTSOCK                    108
#define ETIMEDOUT                   116


#endif