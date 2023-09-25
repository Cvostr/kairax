#include "fcntl.h"
#include "syscalls.h"
#include "errno.h"

int openat(int dirfd, const char* filepath, int flags, int mode)
{
    __set_errno(syscall_open_file(dirfd, filepath, flags, mode));
}

int open(const char* filepath, int flags, int mode)
{
    __set_errno(syscall_open_file(AT_FDCWD, filepath, flags, mode));
}