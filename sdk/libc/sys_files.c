#include "sys_files.h"
#include "../sys/syscalls.h"
#include "errno.h"

int open_file(const char* filepath, int flags, int mode)
{
    __set_errno(syscall_open_file(FD_CWD, filepath, flags, mode));
}

int close(int fd)
{
    __set_errno(syscall_close(fd));
}

ssize_t read(int fd, char* buffer, size_t size)
{
    __set_errno(syscall_read(fd, buffer, size));
}

ssize_t write(int fd, const char* buffer, size_t size)
{
    __set_errno(syscall_write(fd, buffer, size));
}

int fdstat(int fd, struct stat* st)
{
    __set_errno(syscall_fdstat(fd, st));
}

int file_stat(const char* filepath, struct stat* st)
{
    return 0;
}

int readdir(int fd, struct dirent* direntry)
{
    __set_errno(syscall_readdir(fd, direntry));
}

off_t file_seek(int fd, off_t offset, int whence)
{
    __set_errno(syscall_file_seek(fd, offset, whence));
}