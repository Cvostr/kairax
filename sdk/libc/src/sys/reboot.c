#include "sys/reboot.h"
#include "syscalls.h"

int reboot (int flag)
{
    return syscall_poweroff(flag);
}