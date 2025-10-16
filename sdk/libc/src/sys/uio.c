#include "sys/uio.h"
#include "unistd.h"

ssize_t readv(int fd, const struct iovec *iovec, int count)
{
    // TODO: в системный вызов?
    for (int i = 0; i < count; i ++)
    {
        read(fd, iovec[i].iov_base, iovec[i].iov_len);
    }
}

ssize_t writev(int fd, const struct iovec *iovec, int count)
{
    // TODO: в системный вызов?
    for (int i = 0; i < count; i ++)
    {
        write(fd, iovec[i].iov_base, iovec[i].iov_len);
    }
}