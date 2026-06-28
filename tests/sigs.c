#include "stdio.h"
#include "fcntl.h"
#include "unistd.h"
#include "sys/stat.h"
#include "errno.h"
#include "process.h"
#include "arpa/inet.h"
#include "signal.h"
#include "string.h"
#include "stdlib.h"
#include <termios.h>
#include "threads.h"
#include "syscalls.h"
#include "sched.h"
#include "libgen.h"
#include "sys/resource.h"
#include "sys/wait.h"

void segfaulthandle(int arg)
{
    printf("SIGSEGV handled\n");
	exit(1);
}

void pipehandle(int arg)
{
    printf("SIGPIPE handled\n");
}

void chldhandle(int arg)
{
    printf("SIGCHLD handled arg %i\n", arg);

    int status;
    pid_t p = waitpid(-1, &status, WNOHANG);

    printf("Exited process %i with status %i\n", p, WEXITSTATUS(status));
}

void inthandler(int arg)
{
    printf("signal handled with arg %i\n", arg);
	printf("Terminate [y/n]?\n");
	char r;
	scanf("%c", &r);
	printf("input: %c (%i)\n", r, r);
    if (r == 'y')
	{
        exit(0);
	}
}

int main(int argc, char** argv, char** envp) 
{
    if (argc > 1 && strcmp(argv[1], "segv") == 0)
    {
        signal(SIGSEGV, segfaulthandle);
        int *__ptr = 0;
        *__ptr = 0;
    }

    if (argc > 1 && strcmp(argv[1], "pipe") == 0)
    {
        signal(SIGPIPE, pipehandle);
        int pipefds[2];
        pipe(pipefds);
        close(pipefds[0]);
        char ch = 3;
        ssize_t res = write(pipefds[1], &ch, 1);
        printf("write to closed read end returned rc=%i, errno=%i\n", res, errno);
    }

    if (argc > 1 && strcmp(argv[1], "chld") == 0)
    {
        signal(SIGCHLD, chldhandle);
        pid_t ch = fork();
        if (ch == 0)
        {
            printf("Hello from child\n");
            sleep(1);
            exit(55);
        }
        printf("Created process %i\n", ch);
    }

    sighandler_t hd = signal(SIGINT, inthandler);
    if (hd == SIG_ERR)
    {
        perror("signal error");
        return 1;
    }

    sigset_t testset;
    sigemptyset(&testset);
    sigaddset(&testset, 2);
    sigaddset(&testset, 4);
    printf("sigismember %i %i %i %i\n", 
    sigismember(&testset, 1), sigismember(&testset, 2), sigismember(&testset, 3), sigismember(&testset, 4));

    while (1)
    {
        printf("Sigs program echo\n");
        sleep(2);
    }
}