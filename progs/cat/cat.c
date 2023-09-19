#include "../../sdk/libc/stdio.h"
#include "../../sdk/libc/sys_files.h"
#include "../../sdk/sys/syscalls.h"

#define REGION_LEN 15
char region[REGION_LEN];

int main(int argc, char** argv) {

    if (argc < 2) {
        printf("Unspecified file to read\n");
        return 1;
    }

    char* src = argv[1];

    int srcfd = open_file(src, FILE_OPEN_MODE_READ_ONLY, 0);

    if (srcfd == -1) {
        printf("Can't open file %s", src);
        return 2;
    }

    ssize_t readed = 0;
    while ((readed = read(srcfd, region, REGION_LEN)) > 0) {

        for (int i = 0; i < readed; i ++) {
            printf("%c", region[i]);
        }
    }

    close(srcfd);

    return 0;
}