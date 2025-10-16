#ifndef _UIO_H
#define _UIO_H

#include <sys/cdefs.h>
#include "types.h"
#include <stddef.h>

__BEGIN_DECLS

struct iovec
{
    void *iov_base;
    size_t iov_len;
};

ssize_t readv(int fd, const struct iovec *iovec, int count);
ssize_t writev(int fd, const struct iovec *iovec, int count);

__END_DECLS

#endif