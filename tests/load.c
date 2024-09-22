#include "unistd.h"
#include "sys/wait.h"
#include "fcntl.h"
#include "stdio.h"
#include "errno.h"

#define TEST1_PROCS 125
#define TEST2_ROUNDS 300

#define ROUNDS 30000
#define BUFSZ 100

#define RC 127

pid_t pids[TEST1_PROCS];

void procAct() {
    pid_t pid = getpid();
    //fdprintf(STDOUT_FILENO, "%i ", pid);

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

void test1() 
{
    for (int i = 0; i < TEST1_PROCS; i ++) {

        pids[i] = fork();
        if (pids[i] == 0) {
            procAct();
        }
    }

    printf("Created %i processes\n", TEST1_PROCS);

    int status = -1;

    for (int i = 0; i < TEST1_PROCS; i ++) {
        int rc = waitpid(pids[i], &status, 0);
        if (status != RC) {
            printf("PANIC\n");
        }
    }

    printf("Load test 1 finished\n");
}

int main(int argc, char** argv) 
{
    int status;
    printf("Kairax load test\n");
    test1();

    for (int i = 0; i < TEST2_ROUNDS; i ++)
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

    printf("Load test 2 (%i rounds) finished\n", TEST2_ROUNDS);

    return 0;
}