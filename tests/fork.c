#include "stdio.h"
#include "assert.h"
#include "string.h"
#include "unistd.h"
#include "sys/wait.h"
#include "errno.h"
#include "fcntl.h"

#define TESTMSG "Test message"

int main(int argc, char** argv) 
{

    printf("Fork() test program\n");

    int fd = open("/dev/random", O_RDONLY, 0);
    int rc = fcntl(fd, F_GETFD);
    printf("fcntl() GETFD result %i\n", rc);
    rc = fcntl(fd, F_SETFD, FD_CLOEXEC);
    rc = fcntl(fd, F_GETFD);
    printf("fcntl() GETFD 2nd result %i\n", rc);

    pid_t r = fork();
    if (r == 0) {
        printf("CHILD %i\n", getpid());
        int errntest = errno;
        errno = 123;

        char* args[3];
        args[0] = "ls.a";
        args[1] = "-a";
        args[2] = NULL;

        int rc = execve("/ls.a", args, NULL);
        printf("exec() :%i\n", rc);
        return 22;
    } else if (r > 0) {
        printf("PARENT, child: %i\n", r);
        printf("Waiting for completion\n");
        int status = 0;
        waitpid(r, &status, 0);
        printf("Completed with code %i\n", status);
    } else {
        perror("fork()");
    }

    // TEST2 with dup2
    int pipefds[2];
    pipe(pipefds);

    r = fork();
    if (r == 0) {

        dup2(pipefds[1], STDOUT_FILENO);
        close(pipefds[1]);
        printf(TESTMSG);
        fflush(stdout);
        return 0;
    } else {
        char msgbuf[100];
        memset(msgbuf, 0, 100);
        read(pipefds[0], msgbuf, 100);
        printf("Message from child: %s\n", msgbuf);
        if (strcmp(msgbuf, TESTMSG) != 0) {
            printf("ERROR, Strings are not equal\n");
            return 1;
        }
    }

    return 0;
}