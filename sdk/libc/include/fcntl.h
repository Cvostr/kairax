#ifndef _FCNTL_H
#define _FCNTL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "sys/types.h"

#define AT_FDCWD		-2
#define AT_EMPTY_PATH   0x1000

#define O_RDONLY	     00
#define O_WRONLY	     01
#define O_RDWR		     02

#define O_APPEND	    02000
#define O_CREAT	        0100
#define O_DIRECTORY	    0200000
#define O_TRUNC	        01000
#define O_NONBLOCK	    04000
#define O_EXCL		    0200
#define O_CLOEXEC       02000000

#define F_DUPFD         0
#define F_GETFD         1
#define F_SETFD         2
#define F_GETFL         3
#define F_SETFL         4

#define F_GETLK	        5
#define F_SETLK	        6
#define F_SETLKW	    7

#define FD_CLOEXEC	    1

int creat(const char *filepath, mode_t mode);

int openat(int dirfd, const char *filepath, int flags, int mode);

int open(const char *filepath, int flags, int mode);

int fcntl(int fd, int cmd, ...);

#ifdef __cplusplus
}
#endif

#endif