#include "stdio.h"
#include "errno.h"
#include "unistd.h"
#include "sys/stat.h"
#include "string.h"

int main(int argc, char** argv) {

    if (argc < 2)
    {
        printf("mkdir: missing argument\n");
        return 1;
    }

    char* path = argv[1];
    int rc = mkdir(path, 0777);

    if (rc == -1) {
        printf("mkdir: Can't create directory %s : %s\n", path, strerror(errno));
    }

    return 0;
}