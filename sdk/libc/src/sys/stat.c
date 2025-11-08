#include "sys/stat.h"
#include "syscalls.h"
#include "errno.h"
#include "fcntl.h"
#include "stddef.h"

int stat(const char* filepath, struct stat* st)
{
    __set_errno(syscall_fdstat(AT_FDCWD, filepath, st, 0));
}

int fstat(int fd, struct stat* st)
{
    __set_errno(syscall_fdstat(fd, NULL, st, AT_EMPTY_PATH));
}

int fstatat(int dirfd, const char* filepath, struct stat* st, int flags)
{
    __set_errno(syscall_fdstat(dirfd, filepath, st, flags));
}

int mkdir(const char* dirpath, int mode)
{
    __set_errno(syscall_create_directory(AT_FDCWD, dirpath, mode));
}

int chmod(const char* filepath, mode_t mode)
{
    __set_errno(syscall_set_file_mode(AT_FDCWD, filepath, mode, 0));
}

int fchmod(int fd, mode_t mode)
{
    __set_errno(syscall_set_file_mode(fd, NULL, mode, AT_EMPTY_PATH));
}

int fchmodat(int dirfd, const char* filepath, mode_t mode, int flags)
{
    __set_errno(syscall_set_file_mode(dirfd, filepath, mode, flags));
}

int mknod(const char *path, mode_t mode, dev_t dev)
{
    return mknodat(AT_FDCWD, path, mode, dev);
}

int mknodat (int dirfd, const char *path, mode_t mode, dev_t dev)
{
    __set_errno(syscall_mknodat(dirfd, path, mode, dev));
}

int utimensat(int fd, const char* path, const struct timespec times[2], int flag)
{
    __set_errno(syscall_set_fdate(fd, path, times, flag));
}