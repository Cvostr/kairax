#include "proc/syscalls.h"
#include "cpu/cpu_local.h"
#include "kairax/string.h"

#define FD_ZERO(set)        (memset ((void*) (set), 0, sizeof(fd_set)))
#define FD_SET(d, set)	    ((set)->fds_bits[__FDSET_BYTE(d)] |= __MAKE_FDMASK(d))
#define FD_CLR(d, set)	    ((set)->fds_bits[__FDSET_BYTE(d)] &= ~__MAKE_FDMASK(d))
#define FD_ISSET(d, set)	(((set)->fds_bits[__FDSET_BYTE(d)] & __MAKE_FDMASK(d)) != 0)

int sys_select(int n, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout)
{
    struct thread *thread = cpu_get_current_thread();
    struct process *process = thread->process;
    
    if (readfds)
        VALIDATE_USER_POINTER(process, readfds, sizeof(fd_set))
    if (writefds)
        VALIDATE_USER_POINTER(process, writefds, sizeof(fd_set))
    if (exceptfds)
        VALIDATE_USER_POINTER(process, exceptfds, sizeof(fd_set))
    if (timeout)
        VALIDATE_USER_POINTER(process, timeout, sizeof(struct timeval))

    return -1;
}