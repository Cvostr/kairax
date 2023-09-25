#ifndef _FCNTL_H
#define _FCNTL_H

#define AT_FDCWD		-2

int openat(int dirfd, const char* filepath, int flags, int mode);

int open(const char* filepath, int flags, int mode);

#endif