#include "stdio.h"
#include "process.h"
#include "unistd.h"
#include "time.h"
#include "sched.h"

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

void fl(int off) {
    double sd = off;
    double orig = sd;
    double co = 1234;
    while (1) {
        sd = sd * (1 + ((double) off / 10));
                sched_yield();
        sd *= (off * -1 * co) * 1.5;


        sd /= (1.5);
        sd /= (co * off * -1);
        sd /= (1 + ((double) off / 10));
    

        if (sd != orig)
            printf("%c %f %f\n", 'A' + off, orig, sd);
    }
}


int main(int argc, char** argv) {

    asm volatile ("vshufpd $15, %ymm1, %ymm1, %ymm1;");
    create_thread(fl, 2);
    printf("CREATED");
    fl(4);
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