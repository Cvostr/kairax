#ifndef _FCNTL_H
#define _FCNTL_H

#define AT_FDCWD		-2
#define AT_EMPTY_PATH   0x1000

#define O_RDONLY	     00
#define O_WRONLY	     01
#define O_RDWR		     02

#define O_APPEND	    02000
#define O_CREAT	        0100
#define O_DIRECTORY	    0200000

int openat(int dirfd, const char* filepath, int flags, int mode);

int open(const char* filepath, int flags, int mode);

#endif