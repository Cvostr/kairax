#include "unistd.h"
#include "sys/wait.h"
#include "fcntl.h"
#include "stdio.h"
#include "errno.h"

#define THREADS 70
#define ROUNDS 20000
#define BUFSZ 100

#define RC 127

pid_t pids[THREADS];

void procAct() {
    pid_t pid = getpid();
    fdprintf(STDOUT_FILENO, "%i ", pid);

    char buff[BUFSZ];

    if (pid % 3 == 0)
    {
        int fd = open("/dev/random", O_RDWR, 0);
        for  ( int i = 0; i < ROUNDS; i ++) {
            read(fd, buff, BUFSZ);
        }
        close(fd);

    } else {
        //sleep(2);
    }

//    int rc = execvp("ls", NULL);
//    printf("execvp:  %i\n", errno);

    _exit(RC);
}

int main(int argc, char** argv) 
{
    printf("Kairax load test\n");
    for (int i = 0; i < THREADS; i ++) {

        pids[i] = fork();
        if (pids[i] == 0) {
            procAct();
        }
    }

    printf("Created %i processes\n", THREADS);

    int status = -1;

    for (int i = 0; i < THREADS; i ++) {
        int rc = waitpid(pids[i], &status, 0);
        //printf("rc: %i ", status);
        if (status != RC) {
            printf("PANIC\n");
        }
    }

    printf("Load test finished\n");

    for (int i = 0; i < 100; i ++)
    {
        pid_t pid = fork();
        if (pid == 0) {
            _exit(RC);
        } else {
            int rc = waitpid(pid, &status, 0);
            if (status != RC) {
                printf("PANIC\n");
            }
        }
    } 

    return 0;
}