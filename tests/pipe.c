#include "stdio.h"
#include "process.h"
#include "unistd.h"
#include "time.h"
#include "sched.h"

int fds[2];

void thread(void* n) {
    char val[60];
    val[59] = 0;
    while (1) {
        int rc = read(fds[0], &val[0], 58);
        write(STDOUT_FILENO, val, rc);
        //printf("%s\n", val);
        //sleep(1);
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

    pipe(fds);
    create_thread(thread, NULL);
    create_thread(thread2, NULL);

    /*char val = 'A';
    while (1) {
        write(fds[1], &val, 1);
        val += 1;
        if (val == 'I')
            val = 'A';
    }*/
    sleep(1);

    //close(fds[0]);
    //close(fds[1]);

    return 0;
}