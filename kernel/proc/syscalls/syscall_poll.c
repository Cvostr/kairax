#include "proc/syscalls.h"
#include "cpu/cpu_local.h"

int sys_poll(struct pollfd *ufds, nfds_t nfds, int timeout)
{
    struct thread *thread = cpu_get_current_thread();
    struct process *process = thread->process;

    VALIDATE_USER_POINTER(process, ufds, sizeof(struct pollfd) * nfds)

    return -1;
}