#include "stdio.h"
#include "errno.h"
#include "unistd.h"
#include "fcntl.h"
#include "string.h"

#define REGION_LEN 4096 * 3
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
        printf("cp: can't open source file '%s': %s\n", src, strerror(errno));
        return 2;
    }

    // Создаем файл
    int dstfd = creat(dest, 0777);

    if (dstfd == -1) {
        printf("cp: can't open destination file '%s': %s\n", dest, strerror(errno));
        return 3;
    }

    ssize_t readed = 0, written = 0;
    size_t total_written = 0;
    while ((readed = read(srcfd, region, REGION_LEN)) > 0) 
    {
        written = write(dstfd, region, readed);
        if (written < 0)
        {
            printf("cp: error writing to destination file: %s\n", dest, strerror(errno));
            return 1;
        }

        total_written += written;
    }

    printf("File copied, written %i bytes", total_written);

    close(srcfd);
    close(dstfd);

    return 0;
}