#include "stdio.h"
#include "unistd.h"
#include "fcntl.h"
#include "string.h"
#include "errno.h"

#define REGION_LEN 512
char region[REGION_LEN];

int main(int argc, char** argv) 
{
    int srcfd;

    if (argc < 2) 
    {
        srcfd = STDIN_FILENO;
    }
    else
    {
        char* src = argv[1];
        srcfd = open(src, O_RDONLY, 0);

        if (srcfd == -1) {
            printf("cat: Can't open file '%s': %s\n", src, strerror(errno));
            return 2;
        }
    }

    ssize_t readed = 0;
    while ((readed = read(srcfd, region, REGION_LEN)) > 0) 
    {
        write(STDOUT_FILENO, region, readed);
    }

    if (readed < 0) {
        perror("Can't read file");
        return 1;
    }

    close(srcfd);

    return 0;
}