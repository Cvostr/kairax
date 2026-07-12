#include "stdio.h"
#include "errno.h"
#include "unistd.h"
#include "sys/stat.h"
#include "string.h"

int main(int argc, char** argv) 
{
    if (argc < 2)
    {
        printf("mkfifo: missing argument\n");
        return 1;
    }

    char* path = argv[1];

    int rc = mkfifo(path, 0777);

    if (rc == -1) {
        printf("mkfifo: cannot create fifo '%s' : %s\n", path, strerror(errno));
    }

    return 0;
}