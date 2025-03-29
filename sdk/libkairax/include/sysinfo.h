#ifndef _SYSINFO_H
#define _SYSINFO_H

#include "stdint.h"
#include "stddef.h"
#include "time.h"

#define SYSINFO_KERNEL_INFO_STR 1
#define SYSINFO_MEMORY          2
#define SYSINFO_SYSSTAT         3

struct meminfo {
    size_t mem_total;
    size_t mem_used;
    size_t pagesize;
};

struct sysstat {
    time_t uptime;
    size_t processes;
    size_t threads;
};

int KxGetKernelString(char* buffer, size_t buffersz);
int KxGetMeminfo(struct meminfo* info);
int KxGetSysStat(struct sysstat* stat);

#endif