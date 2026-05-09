#include "stdio.h"
#include "unistd.h"
#include "errno.h"
#include "stdio_impl.h"
#include "sys/wait.h"

#define SHELL "/bin/sh"

FILE *popen(const char *command, const char *type)
{
    int pipefds[2];
    FILE *res;
    pid_t pid;
    int write = 0;

    int procfd, streamfd;

    if (type[0] == 'w')
    {
        write = 1;
    } 
    else if (type[0] != 'r')
    {
        errno = EINVAL;
        return NULL;
    }

    // Создаем pipe
    if (pipe(pipefds) < 0)
    {
        return NULL;
    }

    if (write == 1)
    {
        procfd = pipefds[0];
        streamfd = pipefds[1];
    }
    else
    {
        procfd = pipefds[1];
        streamfd = pipefds[0];
    }

    res = fdopen(streamfd, type);
    if (res == NULL)
    {
        close(pipefds[0]);
        close(pipefds[1]);
        return NULL;
    }

    pid = fork();
    if (pid < 0)
    {
        fclose(res);
        close(procfd);
        return NULL;
    }

    if (pid == 0)
    {
        // мы в потомке
        const char* args[4];
        args[0] = SHELL;
        args[1] = "-c";
        args[2] = command;
        args[3] = 0;

        // закрываем ненужный fd для потомка, чтобы не было проблем со счетчиком ссылок
        close(streamfd);

        // Заменим стандартный поток нашим каналом
        dup2(procfd, write == 1 ? STDIN_FILENO : STDOUT_FILENO);
        close(procfd);

        execv(SHELL, (char *const *) args);
        _exit(127);
    }

    // закрываем ненужный fd для родительского процесса, чтобы не было проблем со счетчиком ссылок
    close(procfd);

    res->_popen_pid = pid;

    return res;
}

int pclose(FILE *stream)
{
    int status;
    pid_t pid = stream->_popen_pid;

    fclose(stream);

    if (waitpid(pid, &status, 0) >= 0)
    {
        return status;
    }

    return -1;
}