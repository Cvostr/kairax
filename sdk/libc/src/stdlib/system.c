#include "stdlib.h"
#include "unistd.h"
#include "sys/wait.h"
#include "stdio.h"

char* sh = "/rxsh.a";

int system(const char *command)
{
    int res = -1;
    pid_t pid = fork();

    //todo: implement

    if (pid > 0) {
        waitpid(pid, &res, 0);
    } else {
        const char* args[4];
        args[0] = sh;
        args[1] = "-c";
        args[2] = command;
        args[3] = 0;

        execv(sh, args);
        _exit(127);
    }

	return res;
}