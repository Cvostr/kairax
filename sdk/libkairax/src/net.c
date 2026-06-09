#include "net.h"
#include "ksyscalls.h"
#include "errno.h"

int netctl(int op, int param, struct netinfo* netinfo)
{
    __sys_ret_set_errno(syscall_netctl(op, param, netinfo));
}

int netstat(int index, struct netstat* stat)
{
    __sys_ret_set_errno(syscall_netstat(index, stat));
}