#include "proc/syscalls.h"
#include "mem/pmm.h"
#include "stdio.h"
#include "proc/process.h"
#include "proc/timer.h"
#include "cpu/cpu.h"
#include "cpu/cpu_local.h"
#include "string.h"

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

struct cpuinfo {
    uint16_t arch;
    uint8_t byteorder;
    uint8_t sockets;
    uint16_t cpus;
    char vendor_string[CPU_VENDOR_STR_LEN];
    char model_string[CPU_MODEL_STR_LEN];
};

extern int kairax_version_major;
extern int kairax_version_minor;
extern const char* kairax_build_date;
extern const char* kairax_build_time;

int sys_sysinfo(int request, char* buffer, size_t bufsize)
{
    int rc = 0;
 
    struct thread* thread = cpu_get_current_thread();
    struct process* process = thread->process;

    VALIDATE_USER_POINTER(process, buffer, bufsize)
 
    switch (request)
    {
        case SYSINFO_KERNEL_INFO_STR:
            rc = sprintf(buffer, bufsize, "Kairax Kernel v%i.%i (%s %s)", kairax_version_major, kairax_version_minor, kairax_build_date, kairax_build_time);
            if (rc == -1)
            {
                return -ERROR_RANGE;
            }
            break;
        case SYSINFO_MEMORY:
            if (bufsize != sizeof(struct meminfo))
            {
                return -ERROR_RANGE;
            }
            struct meminfo* minfo = (struct meminfo*) buffer;
            minfo->mem_total = pmm_get_physical_mem_size();
            minfo->mem_used = pmm_get_used_pages() * PAGE_SIZE;
            minfo->pagesize = PAGE_SIZE;
            break;
        case SYSINFO_SYSSTAT:
            if (bufsize != sizeof(struct sysstat))
            {
                return -ERROR_RANGE;
            }
            struct sysstat* sstat = (struct sysstat*) buffer;
            sstat->uptime = timer_get_uptime();
            get_process_count(&sstat->processes, &sstat->threads);
            break;
        case SYSINFO_CPUINFO:
            if (bufsize != sizeof(struct cpuinfo))
            {
                printf("Diff %i %i\n", bufsize, sizeof(struct cpuinfo));
                return -ERROR_RANGE;
            }
            struct cpuinfo* cpuinf = (struct cpuinfo*) buffer;
            
            struct cpu cpu;
            cpu_get_info(&cpu);

            cpuinf->arch = cpu.arch;
            cpuinf->byteorder = cpu.byteorder;
            cpuinf->cpus = cpu.cpus;
            cpuinf->sockets = cpu.sockets;

            strncpy(cpuinf->model_string, cpu.model_string, CPU_MODEL_STR_LEN);
            strncpy(cpuinf->vendor_string, cpu.vendor_string, CPU_VENDOR_STR_LEN);
            break;
        default:
            return -ERROR_INVALID_VALUE;
    }

    return 0;
}