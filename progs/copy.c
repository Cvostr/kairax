#include "../sdk/libc/stdio.h"
#include "../sdk/libc/sys_files.h"
#include "../sdk/sys/syscalls.h"

#define REGION_LEN 9000
char region[REGION_LEN];

int main(int argc, char** argv) {

    if (argc < 3) {
        printf("Unspecified");
        return 1;
    }

    char* src = argv[1];
    char* dest = argv[2];

    int srcfd = open_file(src, FILE_OPEN_MODE_READ_ONLY, 0xFFF);

    if (srcfd == -1) {
        printf("Can't open source file %s", src);
        return 2;
    }

    int dstfd = open_file(dest, FILE_OPEN_FLAG_CREATE | FILE_OPEN_MODE_WRITE_ONLY, 0xFFF);

    if (dstfd == -1) {
        printf("Can't open destination file %s", dest);
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