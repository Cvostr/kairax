#include "stdio.h"
#include "process.h"
#include "unistd.h"
#include "time.h"
#include "sched.h"
#include "errno.h"
#include "sys/wait.h"

int fds[2];

void thread(void* n) {
    char val = '0';
    for (int i = 0; i < 7000; i ++)
    {
        write(fds[1], &val, 1);
        val += 1;
        if (val == '9')
            val = '0';
    }

    close(fds[1]);

    thread_exit(125);
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

    pid_t pid = create_thread(thread, NULL);

    char val[81];
    ssize_t total = 0;
    while (1) {
        ssize_t rrc = read(fds[0], val, 80);
        if (rrc < 0) {
            break;
        }
        total += rrc;
        write(STDOUT_FILENO, val, rrc);
    }

    int status  = 0;
    waitpid(pid, &status,  0);

    printf("Readed %i bytes!\n", total);

    //close(fds[0]);
    //close(fds[1]);

    return 0;
}