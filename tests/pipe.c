#include "stdio.h"
#include "process.h"
#include "unistd.h"
#include "time.h"
#include "sched.h"
#include "errno.h"

int fds[2];

void thread(void* n) {
    char val[60];
    val[59] = 0;
    while (1) {
        int rc = read(fds[0], &val[0], 58);
        //write(STDOUT_FILENO, val, rc);
        //printf("%s\n", val);
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

    int rc = pipe(fds);

    off_t lrc = lseek(fds[0], 1, SEEK_CUR);
    if (lrc != -1) {
        printf("Successful lseek() for pipe, rc = %i\n", lrc);
        return 1;
    }
    if (errno != ESPIPE) {
        printf("Incorrect errno(), expected %i, got %i\n", ESPIPE, errno);
        return 2;
    }

    create_thread(thread, NULL);
    create_thread(thread2, NULL);

    sleep(2);

    //close(fds[0]);
    //close(fds[1]);

    return 0;
}