#include "stdio.h"
#include "string.h"
#include "fcntl.h"
#include "stdlib.h"
#include "unistd.h"
#include "stdio_impl.h"
#include "errno.h"

FILE *__stdio_file_root = NULL;
extern void __fflush_atexit();

int compute_flags(const char *mode) 
{
    int flags = 0;

    while (1) {
        
        switch (*mode) {
            case 'r': 
                flags |= O_RDONLY;
                break;
            case 'a':
                flags |= (O_WRONLY | O_CREAT | O_APPEND);
                break;
            case 'w':
                flags |= (O_WRONLY | O_CREAT | O_TRUNC); 
                break;
            case 'x':
                flags |= O_EXCL;
                break;
            case '+':
                flags |= (flags & (~O_WRONLY)) | O_RDWR;
                break;
            case '\0':
                return flags;
        }

        mode++;
    }

    return 0;
}

FILE *fopen(const char *restrict filename, const char *restrict mode)
{
    if (!strchr("rwa", *mode)) {
        errno = EINVAL;
		return 0;
	}

    int flags = compute_flags(mode);

    int fd = open(filename, flags, 0666);
    
    if (fd == -1) {
        return NULL;
    }

    FILE* file = fdopen(fd, mode);
    if (file) {
        return file;
    }
    
    close(fd);

    return NULL;
}

FILE *__fdopen(int fd, int uflags)
{
    FILE* fil = malloc(sizeof(struct IO_FILE));
    if (fil == NULL) 
    {
        errno = ENOMEM;
        return NULL;
    }

    memset(fil, 0, sizeof(struct IO_FILE));

    fil->_fileno = fd;
    fil->_buf_len = STDIO_BUFFER_LENGTH;
    fil->_buffer = malloc(STDIO_BUFFER_LENGTH);
    if (fil->_buffer == NULL)
    {
        free(fil);
        errno = ENOMEM;
        return NULL;
    }

    switch (uflags & 3) {
        case O_RDWR:
            fil->_flags |= FSTREAM_CANWRITE;
        case O_RDONLY:
            fil->_flags |= (FSTREAM_CANREAD | FSTREAM_INPUT);
            break;
        case O_WRONLY:
            fil->_flags |= FSTREAM_CANWRITE;
            break;
    }

    // Добавим новый FILE в цепочку (для fflush (NULL))
    fil->_next = __stdio_file_root;
    __stdio_file_root = fil->_next;

    // Зарегистрируем очистку всех FILE в atexit
    __fflush_atexit();

    return fil;
}

FILE *fdopen(int fd, const char* restrict mode)
{
    int uflags = compute_flags(mode);

    if (fd < 0) 
    { 
        errno = EBADF;
        return NULL; 
    }

    return __fdopen(fd, uflags);
}

// https://pubs.opengroup.org/onlinepubs/9799919799/functions/freopen.html
FILE *freopen(const char *pathname, const char *mode, FILE *stream)
{
    if (stream == NULL)
    {
        // судя по тому, что POSIX ничего не пишет об этом
        // считаем, что допускается неопределенное поведение
        // но мы всё равно проверим, муахаха
        errno = EINVAL;
        return NULL; 
    }

    if (pathname == NULL)
    {
        // по POSIX в таком случае просто меняем режим файла
        // TODO: попробовать сделать
        errno = EINVAL;
        return NULL; 
    }

    // сбрасываем буфер, на результат не обращаем внимания
    fflush_unlocked(stream);

    // Закрываем файловый дескриптор, на результат также не обращаем внимания
    close(stream->_fileno);

    // очищаем флаги режима и, самое главное, сбрасываем ошибки
    stream->_flags = 0;

    int flags = compute_flags(mode);
    stream->_fileno = open(pathname, flags);
    
    if (stream->_fileno == -1) 
    {
        return NULL;
    }

    switch (flags & 3) {
        case O_RDWR:
            stream->_flags |= FSTREAM_CANWRITE;
        case O_RDONLY:
            stream->_flags |= (FSTREAM_CANREAD | FSTREAM_INPUT);
            break;
        case O_WRONLY:
            stream->_flags |= FSTREAM_CANWRITE;
            break;
    }

    return stream;
}