#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "sys/stat.h"
#include "errno.h"
#include "threads.h"
#include "fcntl.h"
#include <sys/mman.h>
#include <unistd.h>

#define MMAP_PORTION 100

int main(int argc, char** argv)
{
    printf("Test 1: File read to mmaped mem\n");
    int fd = open("/dev/zero", O_RDONLY, 0);
    char* buff = mmap(NULL, MMAP_PORTION, PROT_READ | PROT_WRITE, MAP_ANONYMOUS, -1, 0);
    if (buff == NULL) {
        printf("Failed mmap(), error %i\n", errno);
        return 1;
    }
    int rc = read(fd, buff, MMAP_PORTION);
    if (rc == -1) {
        printf("Failed read(), error %i\n", errno);
        return 2;
    }

    int testsize = 1024 * 1024;
    printf("Test 2: mmap and munmap\n");
    buff = mmap(NULL, testsize, PROT_READ | PROT_WRITE, MAP_ANONYMOUS, -1, 0);
    rc = munmap(buff, testsize);
    if (rc == -1) {
        printf("Failed munmap(), error %i\n", errno);
        return 3;
    }

    printf("Test 3: unmap() with incorrect addr\n");
    rc = munmap(0xFFFFFFFF, testsize);
    if (rc == 0) {
        printf("Successful munmap on 0xFFFFFFFF\n");
        return 4;
    }

    char* addr = (char*)0xFFFFFFFF80000000;
    printf("Test 4: File read to kernel upper mem\n");
    rc = read(fd, addr, 10000);
    if (rc == 0) {
        printf("Successful read to kernel memory, kernel is now broken\n");
        return 5;
    }

    printf("Test 5: getcwd() to kernel upper mem\n");
    char* strr = getcwd(addr, 10000);
    if (strr != NULL) {
        printf("Successful read to kernel memory, kernel is now broken\n");
        return 5;
    }

    printf("Test 6: mmap() to kernel upper mem\n");
    char* brc = mmap(addr, MMAP_PORTION, PROT_READ | PROT_WRITE, MAP_ANONYMOUS, -1, 0);
    if (brc != MAP_FAILED) {
        printf("Successful mmap()\n");
        return 6;
    }
    if (errno != EINVAL) {
        printf("Incorrect errno(), expected %i, got %i\n", EINVAL, errno);
        return 6;
    }

    printf("Test 7: mmap() non anonimous with bad fd\n");
    brc = mmap(NULL, MMAP_PORTION, PROT_READ | PROT_WRITE, 0, -1, 0);
    if (brc != MAP_FAILED) {
        printf("Successful mmap()\n");
        return 7;
    }
    if (errno != EBADF) {
        printf("Incorrect errno(), expected %i, got %i\n", EBADF, errno);
        return 8;
    }

    printf("Test 8: Read larger data\n");
    rc = read(fd, buff, MMAP_PORTION * 1024);
    if (rc >= 0) {
        printf("Successful read()\n");
        return 8;
    }

    printf("Test 9: Read to unwritable memory\n");
    brc = mmap(NULL, MMAP_PORTION, PROT_READ, MAP_ANONYMOUS, -1, 0);
    printf("Allocated at %p\n", brc);
    rc = read(fd, brc, MMAP_PORTION);
    if (rc >= 0) {
        printf("Successful read()\n");
        return 9;
    }

    return 0;
}