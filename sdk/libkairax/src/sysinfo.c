#include "sysinfo.h"
#include "ksyscalls.h"
#include "errno.h"

int KxGetKernelString(char* buffer, size_t buffersz)
{
    __sys_ret_set_errno(syscall_sysinfo(SYSINFO_KERNEL_INFO_STR, buffer, buffersz));
}

int KxGetMeminfo(struct meminfo* info)
{
    __sys_ret_set_errno(syscall_sysinfo(SYSINFO_MEMORY, info, sizeof(struct meminfo)));
}

int KxGetSysStat(struct sysstat* stat)
{
    __sys_ret_set_errno(syscall_sysinfo(SYSINFO_SYSSTAT, stat, sizeof(struct sysstat)));
}

int KxGetCpuInfo(struct cpuinfo* cpu)
{
    __sys_ret_set_errno(syscall_sysinfo(SYSINFO_CPUINFO, cpu, sizeof(struct cpuinfo)));
}