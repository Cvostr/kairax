#include "unistd.h"
#include "syscalls.h"
#include "errno.h"

char* getcwd(char* buf, size_t size)
{
    int rc = syscall_get_working_dir(buf, size);
    if (rc != 0) {
        errno = -rc;
        return NULL;
    }

    return buf;
}