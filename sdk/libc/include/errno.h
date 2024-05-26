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

#define	ENOENT                      2
#define	EBADF		                9
#define	EAGAIN                      11
#define EISDIR                      21
#define EINVAL                      22
#define ENOTSOCK                    108
#define ETIMEDOUT                   116
#define	EBUSY                       16

#endif