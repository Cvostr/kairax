#include "stdio.h"
#include "process.h"
#include "unistd.h"

int fds[2];

void thread(void* n) {
    char val[10];
    val[9] = 0;
    while (1) {
        read(fds[0], &val[0], 9);
        printf("%s ", val);

        sleep(1);
    }
}

void thread2(void* n) {
    char val = '0';
    while (1) {
        write(fds[1], &val, 1);
        val += 1;
        if (val == '9')
            val = '0';
    }
}

int main(int argc, char** argv) {

    //printf("Hello\n");
    double sd;
    while (1) {
        sd = sd * 2 + 2;
        int f = sd / 10;

        sd = sd * 35 * f;
    }
    /*pipe(fds);
    create_thread(thread, NULL);
    create_thread(thread2, NULL);

    char val = 'A';
    while (1) {
        write(fds[1], &val, 1);
        val += 1;
        if (val == 'I')
            val = 'A';
    }*/

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