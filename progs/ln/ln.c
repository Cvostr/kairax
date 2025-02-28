#include "unistd.h"
#include "string.h"
#include "stdio.h"

int main(int argc, char **argv)
{
    int is_sym = 0;
    char* src = NULL;
    char* dst = NULL;

    int i;
    for (i = 1; i < argc; i ++)
    {
        char* arg = argv[i];
        if (arg[0] == '-')
        {
            // удалить - из начала
            char* opt_name = arg + arg[1] == '-' ? 2 : 1;

            if (strcmp(opt_name, "s") == 0 || strcmp(opt_name, "symbolic") == 0)
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

    int rc = link(src, dst);
    if (rc != 0)
    {
        printf("link failed: %i\n");
        return 1;
    }

    return 0;
}