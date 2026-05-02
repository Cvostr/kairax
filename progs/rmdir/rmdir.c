#include "stdio.h"
#include "errno.h"
#include "unistd.h"
#include "string.h"

int main(int argc, char** argv) 
{
    char* path = argv[1];
    int rc = rmdir(path);

    if (rc == -1) 
    {
        printf("rmdir: can't remove %s: %s\n", path, strerror(errno));
    }

    return 0;
}