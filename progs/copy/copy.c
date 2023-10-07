#include "stdio.h"
#include "sys_files.h"
#include "errno.h"
#include "unistd.h"
#include "fcntl.h"

#define REGION_LEN 9000
char region[REGION_LEN];

int main(int argc, char** argv) {

    if (argc < 3) {
        printf("Unspecified");
        return 1;
    }

    char* src = argv[1];
    char* dest = argv[2];

    int srcfd = open(src, O_RDONLY, 0);

    if (srcfd == -1) {
        printf("Can't open source file %s, code=%i\n", src, errno);
        return 2;
    }

    int dstfd = open(dest, O_CREAT | O_WRONLY, 0777);

    if (dstfd == -1) {
        printf("Can't open destination file %s, code=%i\n", dest, errno);
        return 3;
    }

    ssize_t readed = 0;
    size_t bytes_written = 0;
    while ((readed = read(srcfd, region, REGION_LEN)) > 0) {
        write(dstfd, region, readed);
        bytes_written += readed;
    }

    printf("File copied, written %i bytes", bytes_written);

    close(srcfd);
    close(dstfd);

    return 0;
}