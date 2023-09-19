#include "stdio.h"
#include "sys_files.h"
#include "process.h"

int main(int argc, char** argv) {

    printf("MMAPP TEST\n");
    char* addr = (char*) 0x10000000;
    mmap(addr, 4096, PROTECTION_WRITE_ENABLE, 0);
    addr[0] = 'H';
    addr[1] = 'e';
    addr[2] = 'l';
    addr[3] = '\0';

    printf("BEGIN %s\n", addr);
    float fa = 12200.343f;
    double da = 12200.343;

    double ra = 0.5;

    while (ra < 100) {
        ra = ra * 2;
        sleep(30);
        printf("CURR VALUE %i\n", (int)ra);
    }

    printf("END");

    return 0;
}