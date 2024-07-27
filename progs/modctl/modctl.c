#include "stdio.h"
#include "errno.h"
#include "unistd.h"
#include "string.h"
#include "sys/stat.h"
#include "kernel.h"
#include "fcntl.h"
#include "stdlib.h"

#define ACTON_LOAD      1
#define ACTION_UNLOAD   2

int main(int argc, char** argv) 
{
    char* argument = NULL;
    int action = ACTON_LOAD;
    int rc = 0;
    int fd = 0;
    struct stat filestat;
    size_t filesize = 0;
    ssize_t readen = 0;
    int i;
    char* buffer;

    if (argc <= 1) 
    {
        printf("No arguments\n");
        return 1;
    }

    for (i = 1; i < argc;i ++) 
    {
        char* arg = argv[i];
        if (strcmp(arg, "load") == 0) {
            // 
        } else if (strcmp(arg, "unload") == 0 || strcmp(arg, "-u") == 0) {
            action = ACTION_UNLOAD;
        } else {
            argument = arg;
        }
    }

    if (action == ACTON_LOAD)
    {
        rc = stat(argument, &filestat);
        if (rc != 0)  
        {
            perror("Can't stat file");
            return 2;
        }

        filesize = filestat.st_size;

        fd = open(argument, O_RDONLY, 0);
        if (fd < 0) {
            perror("Can't open file");
            return 2;
        }

        buffer = malloc(filesize);
        if (buffer == NULL) {
            printf("No enough memory\n");
            return 2;
        }

        readen = read(fd, buffer, filesize);
        if (readen == -1) {
            perror("Can't read file");
            return 2;
        }

        rc = KxLoadKernelModule(buffer, filesize);
        if (rc != 0) {
            perror("Can't load module");
            return 2;
        }
    } else if (action == ACTION_UNLOAD) 
    {
        rc = KxUnloadKernelModule(argument);
        if (rc != 0) {
            perror("Can't unload module");
            return 2;
        }
    }

    return 0;
}