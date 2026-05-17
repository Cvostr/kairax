#include "fcntl.h"
#include "syscalls.h"
#include "errno.h"
#include "stdarg.h"

int creat(const char *filepath, mode_t mode)
{
    return open(filepath, O_WRONLY | O_CREAT | O_TRUNC, mode);
}

int openat(int dirfd, const char* filepath, int flags, int mode)
{
    __set_errno(syscall_open_file(dirfd, filepath, flags, mode));
}

int open(const char* filepath, int flags, ...)
{
    mode_t mode = 0;

    if ((flags & O_CREAT)) 
    {
        va_list args;
        va_start(args, flags);
        mode = va_arg(args, mode_t);
        va_end(args);
    }

    __set_errno(syscall_open_file(AT_FDCWD, filepath, flags, mode));
}

int fcntl(int fd, int cmd, ...)
{
    va_list va;
    va_start(va,cmd);

    switch (cmd) {
        case F_GETLK:
        case F_SETLK:
        case F_SETLKW:
            return -1;
        default: {

            void* arg = va_arg(va, void*);
            __set_errno(syscall_fcntl(fd, cmd, arg));
        }
    }
}