#include "../sdk/libc/stdio.h"
#include "../sdk/libc/sys_files.h"
#include "../sdk/libc/process.h"
#include "../sdk/sys/syscalls.h"

#define PAGE_PROTECTION_WRITE_ENABLE    0b1
#define PAGE_PROTECTION_EXEC_ENABLE     0b10
#define PAGE_PROTECTION_USER            0b100

int main(int argc, char** argv) {

    printf("MMAPP TEST");
    char* addr = 0x10000000;
    mmap(addr, 4096, PAGE_PROTECTION_WRITE_ENABLE, 0);
    addr[0] = 'H';
    addr[1] = 'e';
    addr[2] = 'l';
    addr[3] = '\0';

    printf("BEGIN");
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