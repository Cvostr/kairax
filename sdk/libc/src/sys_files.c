#include "sys_files.h"
#include "../sys/syscalls.h"
#include "errno.h"
#include "unistd.h"

int open_file(const char* filepath, int flags, int mode)
{
    __set_errno(syscall_open_file(FD_CWD, filepath, flags, mode));
}

ssize_t read(int fd, char* buffer, size_t size)
{
    __set_errno(syscall_read(fd, buffer, size));
}

ssize_t write(int fd, const char* buffer, size_t size)
{
    __set_errno(syscall_write(fd, buffer, size));
}

int stat_at(int dirfd, const char* filepath, struct stat* st)
{
    __set_errno(syscall_fdstat(dirfd, filepath, st, 0));
}

int readdir(int fd, struct dirent* direntry)
{
    __set_errno(syscall_readdir(fd, direntry));
}

int chmod_at(int dirfd, const char* filepath, mode_t mode)
{
    __set_errno(syscall_set_file_mode(dirfd, filepath, mode, 0));
}