#include "stdlib.h"
#include "unistd.h"
#include "sys/wait.h"
#include "stdio.h"

#define SHELL "/bin/sh"

int system(const char *command)
{
    int res = -1;
    pid_t pid, waited;
    
    if (command == NULL)
    {
        // проверка, что оболочка существует в системе
        return system("exit 0") == 0;
    }
    
    pid = fork();

    // todo: блокировка сигналов

    if (pid > 0) 
    {
        waited = waitpid(pid, &res, 0);
        if (waited != pid)
        {
            res = -1;
        }
    } 
    else 
    {
        const char* args[4];
        args[0] = SHELL;
        args[1] = "-c";
        args[2] = command;
        args[3] = 0;

        execv(SHELL, (char *const *) args);
        _exit(127);
    }

	return res;
}