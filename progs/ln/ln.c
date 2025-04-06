#include "unistd.h"
#include "string.h"
#include "stdio.h"

int main(int argc, char **argv)
{
    int rc;
    int is_sym = 0;
    char* src = NULL;
    char* dst = NULL;

    int i;
    for (i = 1; i < argc; i ++)
    {
        char* arg = argv[i];
        if (arg[0] == '-')
        {
            if (strcmp(arg, "-s") == 0 || strcmp(arg, "--symbolic") == 0)
            {
                is_sym = 1;
            }
        } 
        else 
        {
            break;
        }
    }

    if (argc - i < 2)
    {
        printf("Missing file operands\n");
        return 1;
    }

    src = argv[i];
    dst = argv[i + 1];

    if (is_sym)  {
        rc = symlink(src, dst);
    } else {
        rc = link(src, dst);
    }
    if (rc != 0)
    {
        perror("link failed");
        return 1;
    }

    return 0;
}