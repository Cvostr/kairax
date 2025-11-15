#include "stdio.h"
#include "errno.h"
#include "unistd.h"
#include "stdlib.h"
#include "string.h"

int main(int argc, char** argv) {

    char* path = argv[1];
    if (path == NULL)
    {
        printf("realpath: missing operand\n");
        return 1;
    }

    char *abspath = realpath(path, NULL);

    if (abspath == NULL)
    {
        fprintf(stderr, "realpath: %s: %s\n", path, strerror(errno));
        return 1;
    }

    printf("%s\n", abspath);

    return 0;
}