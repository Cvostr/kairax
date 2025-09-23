#include "proc/syscalls.h"

int sys_sethostname(const char *name, size_t len)
{
    printk("sys_sethostname is not implemented!\n");
    return -1;
}

int sys_setdomainname(const char *name, size_t len)
{
    printk("sys_setdomainname is not implemented!\n");
    return -1;
}