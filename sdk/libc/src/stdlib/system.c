#include "stdlib.h"
#include "unistd.h"
#include "sys/wait.h"

int system(const char *command)
{
    int res = -1;
    pid_t pid = fork();

    //todo: implement

    if (pid != 0) {
        waitpid(pid, &res, 0);
    } else {
        return 0;
    }

	return res;
}