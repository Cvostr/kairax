#include "utime.h"
#include <sys/stat.h>
#include "stddef.h"
#include "fcntl.h"

int utime(const char *path, const struct utimbuf *times)
{
    if (times == 0)
    {
        return utimensat(AT_FDCWD, path, NULL, 0);
    }
    else
    {
        struct timespec tss[2];
        tss[0].tv_sec  = times->actime;
        tss[0].tv_nsec = 0;
        tss[1].tv_sec  = times->modtime;
        tss[1].tv_nsec = 0;

        return utimensat(AT_FDCWD, path, tss, 0);
    }
}