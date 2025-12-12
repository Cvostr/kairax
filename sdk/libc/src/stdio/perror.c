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
    DEFINE_ERROR(EINTR, "Interrupted system call"),
    DEFINE_ERROR(EIO, "I/O error"),
    // ...
    DEFINE_ERROR(E2BIG, "Arg list too long"),
    DEFINE_ERROR(ENOEXEC, "Exec format error"),
    DEFINE_ERROR(EBADF, "Bad file number"),
    DEFINE_ERROR(ECHILD, "No child processes"),
    DEFINE_ERROR(EAGAIN, "Try again"),
    DEFINE_ERROR(ENOMEM, "Out of memory"),
    DEFINE_ERROR(EACCES, "Permission denied"),
    DEFINE_ERROR(EBUSY, "Device or resource busy"),
    DEFINE_ERROR(EEXIST, "File exists"),
    DEFINE_ERROR(EXDEV, "Cross-device link"),
    DEFINE_ERROR(ENODEV, "No such device"),
    DEFINE_ERROR(ENOTDIR, "Not a directory"),
    DEFINE_ERROR(EISDIR, "Is a directory"),
    DEFINE_ERROR(EINVAL, "Invalid argument"),
    DEFINE_ERROR(EMFILE, "Too many open files"),
    DEFINE_ERROR(ENOSPC, "No space left on device"),
    DEFINE_ERROR(ESPIPE, "Illegal seek"),
    DEFINE_ERROR(EROFS, "Read-only file system"),
    DEFINE_ERROR(EPIPE, "Broken pipe"),
    DEFINE_ERROR(ERANGE, "Math result not representable"),
    DEFINE_ERROR(ENOSYS, "Function not implemented"),
    DEFINE_ERROR(ENOTEMPTY, "Directory not empty"),
    // ...
    DEFINE_ERROR(ENOTSOCK, "Socket operation on non-socket"),
    DEFINE_ERROR(EAFNOSUPPORT, "Address family not supported by protocol"),
    DEFINE_ERROR(EADDRINUSE, "Address already in use"),
    DEFINE_ERROR(EADDRNOTAVAIL, "Cannot assign requested address"),
    DEFINE_ERROR(ECONNRESET, "Connection reset by peer"),
    DEFINE_ERROR(EISCONN, "Transport endpoint is already connected"),
    DEFINE_ERROR(ENOTCONN, "Transport endpoint is not connected"),
    DEFINE_ERROR(ETIMEDOUT, "Connection timed out"),
    DEFINE_ERROR(ECONNREFUSED, "Connection refused"),
    DEFINE_ERROR(EHOSTUNREACH, "No route to host")
};

char* __unknown_error = "Unknown error";

int max_descs = sizeof(__errors_descriptions) / sizeof(char*);

void perror(const char *msg)
{
    char* errdesc = __unknown_error;
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