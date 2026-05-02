#include "stdio.h"
#include "errno.h"
#include "stdlib.h"
#include "string.h"

int main(int argc, char** argv) 
{
    char *old, *new; 

    if (argc == 1)
    {
        printf("mv: missing source file operand\n");
        return 1;
    }

    if (argc == 2)
    {
        printf("mv: missing destination file operand\n");
        return 1;
    }

    old = argv[1];
    new = argv[2];

    int rc = rename(old, new);

    if (rc == -1) 
    {
        printf("Can't rename from '%s' to '%s': %s\n", old, new, strerror(errno));
        return 1;
    }

    return 0;
}