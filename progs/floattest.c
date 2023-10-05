#include "stdio.h"
#include "sys_files.h"
#include "process.h"
#include "unistd.h"

int fds[2];

void thread(void* n) {
    char val[2];
    val[1] = 0;
    while (1) {
        read(fds[0], &val[0], 1);
        printf("%s ", val);
    }
}

int main(int argc, char** argv) {

    printf("Hello\n");
    pipe(fds);
    create_thread(thread, NULL, NULL);

    char val = 'A';
    while (1) {
        write(fds[1], &val, 1);
        val += 1;
    }

    /*printf("BEGIN %s\n", addr);
    float fa = 12200.343f;
    double da = 12200.343;

    double ra = 0.5;

    while (ra < 100) {
        ra = ra * 2;
        sleep(30);
        printf("CURR VALUE %i\n", (int)ra);
    }

    printf("END");*/

    return 0;
}