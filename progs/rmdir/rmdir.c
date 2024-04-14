#include "stdio.h"
#include "errno.h"
#include "unistd.h"

int main(int argc, char** argv) {

    char* path = argv[1];
    int rc = rmdir(path);

    if (rc == -1) {
        printf("Can't delete %s, error : %i\n", path, errno);
    }

    return 0;
}