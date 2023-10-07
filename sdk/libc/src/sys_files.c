#include "sys_files.h"
#include "../sys/syscalls.h"
#include "errno.h"
#include "unistd.h"

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