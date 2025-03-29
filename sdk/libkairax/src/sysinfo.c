#include "sysinfo.h"
#include "ksyscalls.h"
#include "errno.h"

int KxGetKernelString(char* buffer, size_t buffersz)
{
    __set_errno(syscall_sysinfo(SYSINFO_KERNEL_INFO_STR, buffer, buffersz));
}

int KxGetMeminfo(struct meminfo* info)
{
    __set_errno(syscall_sysinfo(SYSINFO_MEMORY, info, sizeof(struct meminfo)));
}

int KxGetSysStat(struct sysstat* stat)
{
    __set_errno(syscall_sysinfo(SYSINFO_SYSSTAT, stat, sizeof(struct sysstat)));
}