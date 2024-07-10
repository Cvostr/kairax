#include "unistd.h"
#include "syscalls.h"
#include "errno.h"
#include "string.h"
#include "stdlib.h"
#include "limits.h"

extern char** __environ;

// https://linux.die.net/man/3/execvp

#define DEFAULT_PATH "/bin:/usr/bin"

int execve(const char *filename, char *const argv [], char *const envp[])
{
    __set_errno(syscall_execve(filename, argv, envp));
}

int execv(const char *file, char *const argv[])
{
    return execve(file, argv, __environ);
}

int execvp(const char *file, char *const argv[])
{
    // если есть /, то выполняем как есть
    if (strchr(file, '/') != NULL)
    {
        return execv(file, argv);
    }

    char pathbuf[PATH_MAX];

    char* path = getenv("PATH");
    if (path == NULL) 
        path = DEFAULT_PATH;

    int rc = 0;
    char* curpath = path;
    size_t curlen = 0;
    while (1) {

        char* next = strchr(curpath, ':');
        if (next != NULL) {
            curlen = next - curpath;
        } else {
            curlen = strlen(curpath);
        }

        // В буфер записать директорию
        strncpy(pathbuf, curpath, curlen);
        pathbuf[curlen] = '\0';

        // Если нет / на конце, то добавить
        if (pathbuf[strlen(pathbuf) - 1] != '/')
        {
            strcat(pathbuf, "/");
        }

        strcat(pathbuf, file);

        rc = execv(pathbuf, argv);
        if (rc != 0) {
            if (errno != ENOENT) {
                return -1;
            }
        }

        if (next == NULL)
            break;

        curpath = next + 1;
    } 

    return -1;
}