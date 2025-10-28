#include "stdio.h"
#include "errno.h"
#include "sys/mount.h"
#include "string.h"

int main(int argc, char** argv) {

    char* src = NULL;

    for (int i = 1; i < argc; i ++)
    {
        char* arg = argv[i];
        
        if (arg[0] == '-')
        {

        } 
        else 
        {
            if (src == NULL)
            {
                src = arg;
            } 
        }
    }

    if (src == NULL)
    {
        printf("umount: source is not specified!\n");
        return 1;
    }

    int rc = unmount(src, 0);

    if (rc != 0)
    {
        printf("umount: %s: %s\n", src, strerror(errno));
        return 2;
    }

    return 0;
}