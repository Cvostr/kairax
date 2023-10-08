#include "stdio.h"
#include "errno.h"
#include "unistd.h"
#include "sys/stat.h"

int main(int argc, char** argv) {

    char* path = argv[1];
    int rc = mkdir(path, 0777);

    if (rc == -1) {
        printf("Can't create directory %s, error : %i", path, errno);
    }

    return 0;
}