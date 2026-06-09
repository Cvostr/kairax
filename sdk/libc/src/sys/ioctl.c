#include "sys/ioctl.h"
#include "errno.h"
#include "stdint.h"
#include "syscalls.h"

int ioctl(int fd, uint64_t request, uint64_t arg)
{
    __sys_ret_set_errno(syscall_ioctl(fd, request, arg));
}