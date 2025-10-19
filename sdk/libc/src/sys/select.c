#include "sys/select.h"
#include "errno.h"
#include "syscalls.h"

int select(int n, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout)
{
    __set_errno(syscall_select(n, readfds, writefds, exceptfds, timeout));
}

/*
int pselect(int n, fd_set* readfds, fd_set* writefds, fd_set* exceptfds,
            const struct timespec *timeout, const sigset_t *sigmask)
{

}
*/