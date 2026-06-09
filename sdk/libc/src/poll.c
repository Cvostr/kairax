#include "poll.h"
#include "errno.h"
#include "syscalls.h"

int poll(struct pollfd *ufds, nfds_t nfds, int timeout)
{
    __sys_ret_set_errno(syscall_poll(ufds, nfds, timeout));
}