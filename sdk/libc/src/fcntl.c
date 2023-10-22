#include "fcntl.h"
#include "syscalls.h"
#include "errno.h"

int creat(const char *filepath, mode_t mode)
{
    return open(filepath, O_WRONLY | O_CREAT | O_TRUNC, mode);
}

int openat(int dirfd, const char* filepath, int flags, int mode)
{
    __set_errno(syscall_open_file(dirfd, filepath, flags, mode));
}

int open(const char* filepath, int flags, int mode)
{
    __set_errno(syscall_open_file(AT_FDCWD, filepath, flags, mode));
}