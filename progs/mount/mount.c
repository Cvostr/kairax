#include "stdio.h"
#include "errno.h"
#include "sys/mount.h"
#include "string.h"
#include "err.h"

#define MOUNT_STRLEN 512
const static char MOUNTS_PATH[] = "/proc/mounts";

int print_mounts()
{
    char strbuffer[MOUNT_STRLEN];
    FILE *fmounts = fopen(MOUNTS_PATH, "r");
    if (fmounts == NULL)
    {
        err(1, "Error opening %s", MOUNTS_PATH);
    }
    

    char *lptr;
    while ((lptr = fgets(strbuffer, MOUNT_STRLEN, fmounts)) != NULL) 
    {
        printf(lptr);
    }
    
    fclose(fmounts);

    return 0;    
}

int main(int argc, char** argv) {

    char* src = NULL;
    char* dst = NULL;
    char* fstype = NULL;
    int rdonly = 0;

    if (argc == 1)
    {
        return print_mounts();
    }

    for (int i = 1; i < argc; i ++)
    {
        char* arg = argv[i];
        
        if (arg[0] == '-')
        {
            // Обработать аргументы
            if (strcmp(arg, "-r") == 0)
            {
                rdonly = 1;
            }
            else if (strcmp(arg, "-t") == 0)
            {
                fstype = argv[++i];
            }
        } 
        else 
        {
            if (src == NULL)
            {
                src = arg;
            } 
            else
            {
                dst = arg;   
            }
        }
    }

    if (src == NULL)
    {
        printf("source is not specified!\n");
        return 1;
    }

    if (dst == NULL)
    {
        printf("destination is not specified!\n");
        return 1;
    }

    if (fstype == NULL)
    {
        printf("Filesystem type is not specified!\n");
        return 1;
    }

    int rc = mount(src, dst, fstype, 0);

    if (rc != 0)
    {
        perror("Unable to mount");
        return 2;
    }

    return 1;
}