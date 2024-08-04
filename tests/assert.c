#include "stdio.h"
#include "assert.h"
#include "string.h"
#include "unistd.h"
#include "sys/wait.h"
#include "signal.h"

int main(int argc, char** argv) {

    int rc = 0;
    printf("SIGSEGV test on dereference of NULL\n");
    pid_t pid = fork();
    if (pid == 0) {
        *((char*) 0) = 1;
        printf("Something strange ...\n");
        return -2;
    } else {
        waitpid(pid, &rc, 0);
        printf("child code %i\n", rc);
        if (WSTOPSIG(rc) != SIGSEGV)
        {
            printf("Incorrect result code %i\n", rc);
            return -1;
        }
    }

    printf("Asserion test\n");

    pid = fork();
    if (pid == 0) {
        int i = 1000;
        assert(i == 1);
    } else {
        waitpid(pid, &rc, 0);
        printf("child code %i\n", rc);
        if (WSTOPSIG(rc) != SIGABRT)
        {
            printf("Incorrect result code %i\n", rc);
            return -1;
        }
    }

    return -1;
}