#ifndef _ERRNO_H
#define _ERRNO_H

#define ERROR_NO_FILE               2
#define ERROR_ARGS_BUFFER_BIG       7
#define ERROR_BAD_EXEC_FORMAT       8
#define ERROR_BAD_FD                9
#define	ERROR_NO_CHILD		        10
#define ERROR_NO_MEMORY             12
#define ERROR_IS_DIRECTORY          21
#define ERROR_INVALID_VALUE         22
#define ERROR_NOT_A_DIRECTORY       20        
#define ERROR_TOO_MANY_OPEN_FILES   24
#define ERROR_NO_SPACE              28
#define ERROR_FS_READONLY           30
#define ERROR_RANGE                 34
#define ERROR_WRONG_FUNCTION        38
#define ERROR_ALREADY_EXISTS        17
#define ERROR_OTHER_DEVICE          18
#define	ERROR_BUSY		            16
#define ERROR_NOT_EMPTY             39
#define ERROR_NOT_SOCKET            88

#define	EPERM                       1
#define	ENOENT                      ERROR_NO_FILE
#define	ESRCH		                3
#define	EINTR                       4
#define	E2BIG                       7
#define	EAGAIN                      11
#define	ENOMEM                      ERROR_NO_MEMORY
#define	EACCES		                13
#define	EEXIST		                17
#define EINVAL                      ERROR_INVALID_VALUE
#define	ESPIPE		                29
#define EPIPE		                32
#define	ENOSPC                      ERROR_NO_SPACE
#define ENOTEMPTY                   ERROR_NOT_EMPTY
#define ENOTSOCK                    ERROR_NOT_SOCKET

#define EADDRINUSE	    98
#define EADDRNOTAVAIL	99
#define ECONNRESET	    104
#define EISCONN         106
#define ENOTCONN        107
#define ECONNREFUSED	111
#define ENETUNREACH     113

#endif