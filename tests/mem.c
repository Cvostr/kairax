#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "sys/stat.h"
#include "errno.h"
#include "threads.h"
#include "fcntl.h"
#include <sys/mman.h>
#include <unistd.h>

int main(int argc, char** argv)
{
    printf("Test 1: File read to mmaped mem\n");
    int fd = open("/dev/zero", O_RDONLY, 0);
    char* buff = mmap(NULL, 100, PROT_READ | PROT_WRITE, MAP_ANONYMOUS, -1, 0);
    if (buff == NULL) {
        printf("Failed mmap(), error %i\n", errno);
        return 1;
    }
    int rc = read(fd, buff, 100);
    if (rc == -1) {
        printf("Failed read(), error %i\n", errno);
        return 2;
    }


    char* addr = 0xFFFFFFFF80000000;
    printf("Test 2: File read to kernel upper mem\n");
    rc = read(fd, addr, 10000);
    if (rc == 0) {
        printf("Successful read to kernel memory, kernel is now broken\n");
        return 3;
    }

    return 0;
}