#ifndef _SYSINFO_H
#define _SYSINFO_H

#include "stdint.h"
#include "stddef.h"
#include "time.h"

#define SYSINFO_KERNEL_INFO_STR 1
#define SYSINFO_MEMORY          2
#define SYSINFO_SYSSTAT         3
#define SYSINFO_CPUINFO         4

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

#define CPU_VENDOR_STR_LEN   20
#define CPU_MODEL_STR_LEN   50

#define CPU_ARCH_X86_64     1
#define CPU_ARCH_AARCH64    2
#define CPU_ARCH_RISCV      3

#define CPU_BYTEORDER_BE    1
#define CPU_BYTEORDER_LE    2
struct cpuinfo {
    uint16_t arch;
    uint8_t byteorder;
    uint8_t sockets;
    uint16_t cpus;
    char vendor_string[CPU_VENDOR_STR_LEN];
    char model_string[CPU_MODEL_STR_LEN];
};

int KxGetKernelString(char* buffer, size_t buffersz);
int KxGetMeminfo(struct meminfo* info);
int KxGetSysStat(struct sysstat* stat);
int KxGetCpuInfo(struct cpuinfo* cpu);

#endif