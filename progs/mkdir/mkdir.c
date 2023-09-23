#include "stdio.h"
#include "sys_files.h"
#include "errno.h"
#include "unistd.h"

int main(int argc, char** argv) {

    char* path = argv[1];
    int rc = mkdir(path, 0xFFF);

    if (rc == -1) {
        printf("Can't create directory %s, error : %i", path, errno);
    }

    return 0;
}