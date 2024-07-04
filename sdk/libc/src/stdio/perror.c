#include "stdio.h"
#include "errno.h"
#include "string.h"
#include "unistd.h"

#define DEFINE_ERROR(num, msg) [num] = msg

char* __errors_descriptions[] = {
    DEFINE_ERROR(0x0, "Success"),
    DEFINE_ERROR(EPERM, "Operation not permitted"),
    DEFINE_ERROR(ENOENT, "No such file or directory"),
    DEFINE_ERROR(ESRCH, "No such process"),
    // ...
    DEFINE_ERROR(E2BIG, "Arg list too long"),
    DEFINE_ERROR(ENOEXEC, "Exec format error"),
    DEFINE_ERROR(EBADF, "Bad file number"),
    DEFINE_ERROR(ECHILD, "No child processes"),
    DEFINE_ERROR(ENOMEM, "Out of memory"),
    DEFINE_ERROR(EACCES, "Permission denied"),
    DEFINE_ERROR(EEXIST, "File exists"),
    DEFINE_ERROR(ENOTDIR, "Not a directory"),
    DEFINE_ERROR(EISDIR, "Is a directory"),
    DEFINE_ERROR(EINVAL, "Invalid argument"),
    DEFINE_ERROR(EMFILE, "Too many open files")
};

int max_descs = sizeof(__errors_descriptions) / sizeof(char*);

void perror(const char *msg)
{
    char* errdesc = "Unknown error";
    if (errno < max_descs) {
        errdesc = __errors_descriptions[errno];
    }

    if (msg) {
        write(STDERR_FILENO, msg, strlen(msg));
        write(STDERR_FILENO, ": ", 2);
    }

    write(STDERR_FILENO, errdesc, strlen(errdesc));
    write(STDERR_FILENO, "\n", 1);
}