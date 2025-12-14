#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "fcntl.h"

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