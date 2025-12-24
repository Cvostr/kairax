#include "sys/statfs.h"
#include "syscalls.h"
#include "errno.h"

int statfs(const char *path, struct statfs *buf)
{
    __set_errno(syscall_statfs(path, buf));
}

int fstatfs(int fd, struct statfs *buf)
{
    __set_errno(syscall_fstatfs(fd, buf));
}