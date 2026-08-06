#include "stdio.h"
#include "errno.h"
#include "unistd.h"
#include "fcntl.h"
#include "string.h"
#include "stdlib.h"
#include "limits.h"
#include "libgen.h"

#define REGION_LEN (4096 * 64)

int main(int argc, char** argv) {

    if (argc < 3) {
        printf("Unspecified");
        return 1;
    }

    char* src = argv[1];
    char* dest = argv[2];

    int srcfd = open(src, O_RDONLY, 0);

    if (srcfd == -1) {
        printf("cp: can't open source file '%s': %s\n", src, strerror(errno));
        return 2;
    }

    // Создаем файл
    int dstfd = creat(dest, 0777);

    if (dstfd == -1) 
    {
        if (errno == EISDIR)
        {
            // получим имя исходного файла
            // !!! src будет изменена !!!
            char *src_fname = basename(src);
            // память под временную строку нового пути
            char *newpath = malloc(PATH_MAX);
            // склеим строки
            snprintf(newpath, PATH_MAX, "%s/%s", dest, src_fname);
            // пробуем еще раз открыть новый файл
            dstfd = creat(newpath, 0777);
            // временная строка - новый путь назначения
            dest = newpath;
        }
        
        if (dstfd == -1)
        {
            printf("cp: can't open destination file '%s': %s\n", dest, strerror(errno));
            return 3;
        }
    }

    // Выделим память под регион
    char *buffer = malloc(REGION_LEN);
    if (buffer == NULL) 
    {
        printf("cp: failed to allocate memory\n");
        return 1;
    }

    ssize_t readed = 0, written = 0;
    size_t total_written = 0;
    while ((readed = read(srcfd, buffer, REGION_LEN)) > 0) 
    {
        written = write(dstfd, buffer, readed);
        if (written < 0)
        {
            printf("cp: error writing to destination file '%s': %s\n", dest, strerror(errno));
            free(buffer);
            return 1;
        }

        total_written += written;
    }

    if (readed < 0)
    {
        printf("cp: error reading from source file '%s': %s\n", dest, strerror(errno));
        free(buffer);
        return 1;
    }

    //printf("File copied, written %i bytes", total_written);

    free(buffer);
    close(srcfd);
    close(dstfd);

    return 0;
}