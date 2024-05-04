#include "netctl.h"
#include "ksyscalls.h"

int netctl(int op, int param, struct netinfo* netinfo)
{
    return syscall_netctl(op, param, netinfo);
}