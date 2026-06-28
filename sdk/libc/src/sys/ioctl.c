#include "sys/ioctl.h"
#include "errno.h"
#include "stdint.h"
#include <stdarg.h>
#include "syscalls.h"

int ioctl(int fd, uint64_t request, ...)
{
    unsigned long long arg;

    va_list args;
    va_start(args, request);
    arg = va_arg(args, unsigned long long);
    va_end(args);

    __sys_ret_set_errno(syscall_ioctl(fd, request, arg));
}