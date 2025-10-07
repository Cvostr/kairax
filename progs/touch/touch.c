#include "stdio.h"
#include "unistd.h"
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

    fd = open(filpath, O_WRONLY | O_CREAT | O_APPEND, 0666);
    if (fd == -1)
    {
        perror("touch");
        return 1;
    }

    close(fd);

    return 0;
}