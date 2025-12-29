#include "stdio.h"
#include "unistd.h"
#include "getopt.h"
#include "sys/statfs.h"
#include "err.h"
#include "limits.h"
#include "stdint.h"
#include "stdlib.h"
#include "math.h"

#define MOUNT_STRLEN 1024
const static char MOUNTS_PATH[] = "/proc/mounts";

char dimensions[] = {
    'B',
    'k',
    'M',
    'G',
    'T'
};
#define NDIMENSIONS (sizeof(dimensions) / sizeof(char))

double convert_dimension(uint64_t val, int power, char* dim)
{
    int dim_counter = 0;
    uint64_t last_val = val;
    uint64_t divisor = 1;

    while ((last_val / power) > 0)
    {
        last_val /= power;
        dim_counter++;
        divisor *= power;
    }

    *dim = dimensions[dim_counter];
    return ((double) val) / ((double)divisor);
}

void fill_blocks_info(struct statfs *stat, uint64_t *total, uint64_t *free, uint64_t *used)
{
    *total = stat->f_bsize * stat->f_blocks;
    *free = stat->f_bsize * stat->f_bfree;
    *used = stat->f_bsize * (stat->f_blocks - stat->f_bfree);
}

void fill_inodes_info(struct statfs *stat, uint64_t *total, uint64_t *free, uint64_t *used)
{
    *total = stat->f_files;
    *free = stat->f_ffree;
    *used = (stat->f_files - stat->f_ffree);
}


int main(int argc, char** argv) 
{
    int show_inodes = 0;
    int POWER = 1;

    int c;
    while ((c = getopt (argc, argv, "ihH")) != -1)
    switch (c)
    {
        case 'i':
            show_inodes = 1;
            break;
        case 'h':
            POWER = 1024;
            break;
        case 'H':
            POWER = 1000;
            break;
    }

    int rc;
    char strbuffer[MOUNT_STRLEN];
    FILE *fmounts = fopen(MOUNTS_PATH, "r");
    if (fmounts == NULL)
    {
        err(1, "Error opening %s", MOUNTS_PATH);
    }
    
    char* device = malloc(PATH_MAX);
    char fstype[100];
    char mountpath[PATH_MAX];

    struct statfs statf;
    char *lptr;
    while ((lptr = fgets(strbuffer, MOUNT_STRLEN, fmounts)) != NULL) 
    {
        sscanf(lptr, "%s %s %s", device, mountpath, fstype);
        rc = statfs(mountpath, &statf);
        if (rc != 0)
        {
            warn("statfs: error on %s", mountpath);
            continue;
        }

        uint64_t totalsz;
        uint64_t free;
        uint64_t used;
        
        if (show_inodes == 1)
        {
            fill_inodes_info(&statf, &totalsz, &free, &used);
        }
        else
        {
            fill_blocks_info(&statf, &totalsz, &free, &used);
        }

        int usage_percent = ((double) used / totalsz) * 100;

        if (POWER == 1)
        {
            printf("%s  %lu %lu %lu  %i%%  %s\n", 
                device, 
                totalsz, 
                used,
                free,
                usage_percent,
                mountpath
            );
        }
        else
        {
            double totalsz_val;
            char totalsz_dimension;
            double used_val;
            char used_dimension;
            double free_val;
            char free_dimension;

            totalsz_val = convert_dimension(totalsz, POWER, &totalsz_dimension);
            used_val = convert_dimension(used, POWER, &used_dimension);
            free_val = convert_dimension(free, POWER, &free_dimension);

            printf("%s  %.1f%c %.1f%c %.1f%c  %i%%  %s\n", 
                device, 
                totalsz_val, totalsz_dimension, 
                used_val, used_dimension,
                free_val, free_dimension,
                usage_percent,
                mountpath
            );
        }

    }
    
    fclose(fmounts);
}