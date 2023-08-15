#include "../sdk/libc/stdio.h"
#include "../sdk/libc/sys_files.h"
#include "../sdk/libc/process.h"


int main(int argc, char** argv) {

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