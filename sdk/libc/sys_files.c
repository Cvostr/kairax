#include "sys_files.h"
#include "../sys/syscalls.h"

int open_file(const char* filepath, int flags, int mode)
{
    return syscall_open_file(filepath, flags, mode);
}

int close(int fd)
{
    return syscall_close(fd);
}

int read(int fd, char* buffer, size_t size)
{
    return syscall_read(fd, buffer, size);
}

int fdstat(int fd, struct stat* st)
{
    return syscall_fdstat(fd, st);
}

int file_stat(const char* filepath, struct stat* st)
{
    return 0;
}