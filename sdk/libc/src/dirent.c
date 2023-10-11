#include "dirent.h"
#include "syscalls.h"
#include "errno.h"
#include "unistd.h"

int readdir(int fd, struct dirent* direntry)
{
    __set_errno(syscall_readdir(fd, direntry));
}