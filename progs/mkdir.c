#include "../sdk/libc/stdio.h"
#include "../sdk/libc/sys_files.h"
#include "../sdk/libc/errno.h"

int main(int argc, char** argv) {

    char* path = argv[1];
    int rc = mkdir(path, 0xFFF);

    if (rc == -1) {
        printf("Can't create directory %s, error : %i", path, errno);
    }

    return 0;
}