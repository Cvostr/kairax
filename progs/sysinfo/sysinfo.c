#include "stdio.h"
#include "kairax.h"
#include "string.h"
#include "errno.h"

#define MEM_MB (1024 * 1024)

int main(int argc, char** argv) 
{
    int rc = 0;
    char kname_buffer[100];
    rc = KxGetKernelString(kname_buffer, sizeof (kname_buffer));
    if (rc != 0) 
    {
        perror("Error getting kernel string");
        return 1;
    }
    printf("Kernel: %s\n", kname_buffer);

    struct meminfo meminf;
    rc = KxGetMeminfo(&meminf);
    if (rc != 0) 
    {
        perror("Error getting memory info");
        return 1;
    }
    printf("Memory: Total %i MiB, Used %i MiB\n", meminf.mem_total / MEM_MB, meminf.mem_used / MEM_MB);
    
    struct sysstat sstat;
    rc = KxGetSysStat(&sstat);
    if (rc != 0) 
    {
        perror("Error getting system statistics");
        return 1;
    }
    printf("Uptime: %i sec\n", sstat.uptime);
    printf("Processes: %i Threads: %i\n", sstat.processes, sstat.threads);

    struct cpuinfo cpuinfo;
    rc = KxGetCpuInfo(&cpuinfo);
    if (rc != 0) 
    {
        perror("Error getting CPU information");
        return 1;
    }
    printf("CPU: %s (%s), %i cores\n", cpuinfo.model_string, cpuinfo.vendor_string, cpuinfo.cpus);

    return 0;
}