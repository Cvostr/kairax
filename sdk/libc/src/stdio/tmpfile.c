#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "fcntl.h"
#include "errno.h"
#include <sys/stat.h>

extern FILE *__fdopen(int fd, int uflags);

FILE *tmpfile(void) 
{
    int fd;
    char template[] = "/tmp/tmpfile-XXXXXX";
  
    if ((fd = mkstemp(template)) < 0)
    {
        return NULL;
    }

    // сразу уничтожаем все ссылки. inode будет уничтожена после закрытия файла
    unlink(template);
    return __fdopen(fd, O_RDWR);
}

char *tmpnam(char *s)
{
    // для случаев, когда s = NULL
    static char tmpbuf[100];
    struct stat st;

    if (s == NULL)
        s = tmpbuf;

    while (1)
    {
        sprintf(s, "/tmp/temp_%04x", rand() % 0xFFFF);
        if (stat(s, &st) == -1 && errno == ENOENT)
        {
			return s;
        }
    }

    return NULL;
}