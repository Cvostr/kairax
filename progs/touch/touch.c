#include "stdio.h"
#include "unistd.h"
#include "fcntl.h"
#include "errno.h"
#include "string.h"
#include "sys/stat.h"
#include "fcntl.h"

int main(int argc, char** argv) 
{
    int fd;
    char* filpath = NULL;

    for (int i = 1; i < argc; i ++)
    {
        char* arg = argv[i];
        
        if (arg[0] == '-')
        {
            // TODO: аргументы
        } 
        else 
        {
            filpath = arg;
        }
    }

    if (filpath == NULL)
    {
        printf("touch: missing file operand\n");
        return 1;
    }

    fd = open(filpath, O_WRONLY | O_APPEND, 0666);
    if (fd < 0)
    {
        if (errno == ENOENT)
        {
            fd = open(filpath, O_WRONLY | O_CREAT | O_APPEND, 0666);
            if (fd < 0)
            {
                printf("touch: error creating file '%s': %s", filpath, strerror(errno));
                return 1;
            }
        } 
        else
        {
            printf("touch: error opening file '%s': %s", filpath, strerror(errno));
            return 1;
        }
    }
    else
    {
        if (utimensat(fd, NULL, NULL, AT_EMPTY_PATH) != 0)
        {
            printf("touch: error updating file date: %s\n", strerror(errno));
            return 1;
        }
    }

    close(fd);

    return 0;
}