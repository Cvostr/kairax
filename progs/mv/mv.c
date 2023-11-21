#include "stdio.h"
#include "errno.h"
#include "stdlib.h"

int main(int argc, char** argv) {

    char* old = argv[1];
    char* new = argv[2];
    int rc = rename(old, new);

    if (rc == -1) {
        printf("Can't rename from %s to %s, error : %i\n", old, new, errno);
    }

    return 0;
}