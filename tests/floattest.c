#include "stdio.h"
#include "process.h"
#include "unistd.h"
#include "time.h"
#include "sched.h"
#include "sys/wait.h"
#include "math.h"

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

void fl(int off) 
{
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

void thread433(void* n) 
{
    asm volatile ("mov $57, %rax");
    asm volatile ("movq %rax, %xmm1");
    asm volatile ("movq %rax, %xmm15");

    thread_exit(12);    
}

int main(int argc, char** argv) 
{
    //sin 0.479426 // tan 30 = 0,57735026
    printf("sin(0.5) = %f, cos(60) = %f, tan (30) = %f\n", sin(0.5), cos(60.0 * M_PI / 180), tan(30.0 * M_PI / 180));
    printf("floor(2.12) = %f\n", floor(2.12));
    printf("sqrt(4) = %f, sqrt(6.25) = %f\n", sqrt(4), sqrt(6.25));
    //printf("sqrt(4) = %f, sqrt(25) = %f, sqrt(6.25) = %f\n", sqrt(4), sqrt(25), sqrt(6.25));

    // atan2 1.190290 (0.5, 0.2)
    printf("atan2(0.5, 0.2) = %f\n", atan2(0.5, 0.2));
    printf("pow(3,2) = %f, pow(2.5, 1.5) = %f\n", pow(3, 2), pow(2.5, 1.5));
    // 0.123000 0.281000
    printf("fmod(0.543, 0.21) = %f fmod(5.543, 0.877) = %f\n", fmod(0.543, 0.21), fmod(5.543, 0.877));

    printf("acos(0) = %f acos(1) = %f acos(0.5) = %f\n", acos(0), acos(1), acos(0.5));

    printf("asin(0) = %f asin(1) = %f asin(0.5) = %f\n", asin(0), asin(1), asin(0.5));

    asm volatile ("vshufpd $15, %ymm1, %ymm1, %ymm1;");

    asm volatile ("mov $12, %rax");
    asm volatile ("movq %rax, %xmm1");
    asm volatile ("movq %rax, %xmm15");

    pid_t pid = create_thread(thread433, 2);

    sched_yield();
    int status = 0;
    waitpid(pid, &status, 0);

    int aa = 0;
    asm volatile ("movq %xmm1, %rax");
    asm volatile ("mov %%rax, %0" : "=m" (aa));
    printf("VAL = %i\n", aa);

    asm volatile ("movq %xmm15, %rax");
    asm volatile ("mov %%rax, %0" : "=m" (aa));
    printf("VAL = %i\n", aa);

    //create_thread(fl, 2);
    //printf("CREATED");
    //fl(4);
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