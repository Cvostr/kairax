#include "proc/syscalls.h"
#include "cpu/cpu_local.h"
#include "proc/nodename.h"
#include "string.h"

extern char nodename[];
extern char domainname[];

int sys_sethostname(const char *name, size_t len)
{
    struct thread* thread = cpu_get_current_thread();
    struct process* process = thread->process;

    if (thread->is_userspace) {
        VALIDATE_USER_POINTER(process, name, len)
    }

    if (len > HOSTNAME_MAX)
    {
        return -EINVAL;
    }

    if (process->euid != 0)
    {
        return -EPERM;
    }

    memcpy(nodename, name, len);
    nodename[len] = '\0';

    return 0;
}

int sys_setdomainname(const char *name, size_t len)
{
    struct thread* thread = cpu_get_current_thread();
    struct process* process = thread->process;

    VALIDATE_USER_POINTER(process, name, len)

    if (len > DOMAINNAME_MAX)
    {
        return -EINVAL;
    }

    if (process->euid != 0)
    {
        return -EPERM;
    }

    memcpy(domainname, name, len);
    domainname[len] = '\0';

    return 0;
}