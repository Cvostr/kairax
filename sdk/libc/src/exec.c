#include "unistd.h"
#include "syscalls.h"
#include "errno.h"
#include "string.h"
#include "stdlib.h"
#include "limits.h"
#include "stdarg.h"

extern char** __environ;

// https://linux.die.net/man/3/execvp

#define DEFAULT_PATH "/bin:/usr/bin"

int execve(const char *filename, char *const argv [], char *const envp[])
{
    __sys_ret_set_errno(syscall_execve(filename, argv, envp));
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

int execl(const char *path, const char* arg, ...)
{
    va_list ap, copy;
    int rc;
    size_t nargs = 0;
    va_start(ap, arg);
    va_copy(copy, ap);

    while (va_arg(copy, char*))
        nargs++;
    va_end(copy);

    char **argv = malloc((nargs + 2) * sizeof(char *));
    if (argv == NULL)
    {
        va_end(ap);
        errno = ENOMEM;
        return -1;
    }

    argv[0] = (char*) arg;
    argv[nargs + 1] = NULL;
    for (size_t i = 0; i < nargs; i ++)
        argv[i + 1] = va_arg(ap, char*);
    
    va_end(ap);

    rc = execve(path, argv, __environ);

    // если мы сюда попали, значит произошла ошибка в execve
    free(argv);
    return rc;
}

int execlp(const char *file, const char *arg, ...)
{
    va_list ap, copy;
    int rc;
    size_t nargs = 0;
    va_start(ap, arg);
    va_copy(copy, ap);

    while (va_arg(copy, char*))
        nargs++;
    va_end(copy);

    char **argv = malloc((nargs + 2) * sizeof(char *));
    if (argv == NULL)
    {
        va_end(ap);
        errno = ENOMEM;
        return -1;
    }

    argv[0] = (char*) arg;
    argv[nargs + 1] = NULL;
    for (size_t i = 0; i < nargs; i ++)
        argv[i + 1] = va_arg(ap, char*);
    
    va_end(ap);

    rc = execvp(file, argv);

    // если мы сюда попали, значит произошла ошибка в execvp
    free(argv);
    return rc;
}