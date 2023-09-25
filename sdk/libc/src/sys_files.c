#include "sys_files.h"
#include "../sys/syscalls.h"
#include "errno.h"
#include "unistd.h"

int open_file_at(int dirfd, const char* filepath, int flags, int mode)
{
    __set_errno(syscall_open_file(dirfd, filepath, flags, mode));
}

int open_file(const char* filepath, int flags, int mode)
{
    __set_errno(syscall_open_file(FD_CWD, filepath, flags, mode));
}

int mkdir(const char* dirpath, int mode)
{
    __set_errno(syscall_create_directory(FD_CWD, dirpath, mode));
}

ssize_t read(int fd, char* buffer, size_t size)
{
    __set_errno(syscall_read(fd, buffer, size));
}

ssize_t write(int fd, const char* buffer, size_t size)
{
    __set_errno(syscall_write(fd, buffer, size));
}

int fstat(int fd, struct stat* st)
{
    __set_errno(syscall_fdstat(fd, "", st, DIRFD_IS_FD));
}

int stat(const char* filepath, struct stat* st)
{
    __set_errno(syscall_fdstat(FD_CWD, filepath, st, 0));
}

int stat_at(int dirfd, const char* filepath, struct stat* st)
{
    __set_errno(syscall_fdstat(dirfd, filepath, st, 0));
}

int readdir(int fd, struct dirent* direntry)
{
    __set_errno(syscall_readdir(fd, direntry));
}

int chmod(const char* filepath, mode_t mode)
{
    __set_errno(syscall_set_file_mode(FD_CWD, filepath, mode, 0));
}

int chmod_at(int dirfd, const char* filepath, mode_t mode)
{
    __set_errno(syscall_set_file_mode(dirfd, filepath, mode, 0));
}