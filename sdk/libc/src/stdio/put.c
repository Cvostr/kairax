#include "stdio.h"
#include "stdio_impl.h"
#include "unistd.h"
#include "string.h"

int putchar(int c) 
{
    return fputc(c, stdout);
}

int __puts(const char* s, size_t size) 
{
    return (write(STDOUT_FILENO, s, size) == size) ? 1 : 0;
}

int puts(const char* str)
{
    return __puts(str, strlen(str)) && __puts("\n", 1) ? 0 : -1;
}

int fputs(const char *s, FILE* stream)
{
    return fwrite(s, strlen(s), 1, stream);
}

int fputc(int c, FILE *stream)
{
    int tmp;
    
    mtx_lock(&stream->_lock);
    tmp = fputc_unlocked(c, stream);
    mtx_unlock(&stream->_lock);
    
    return tmp;
}

int fputc_unlocked(int c, FILE *stream)
{
    if ((stream->_flags & FSTREAM_CANWRITE) == 0) {
        stream->_flags |= FSTREAM_ERROR;
        return EOF;
    }

    if (stream->_buf_pos >= stream->_buf_len - 1) {
        // Буфер полон - сбрасываем
        if(fflush_unlocked(stream) != 0) {
            stream->_flags |= FSTREAM_ERROR;
            return EOF;
        }
    }

    char val = c;

    if ((stream->_flags & FSTREAM_UNBUFFERED) == FSTREAM_UNBUFFERED) {
        // Поток не буферный - просто пишем в файл
        if (write(stream->_fileno, &val, 1) != 1) {
            stream->_flags |= FSTREAM_ERROR;
            return EOF;
        }

        return c;
    }

    // Запись в буфер
    stream->_buffer[stream->_buf_pos ++] = val;

    if (c == '\n') {
        // Встретили перенос строки - сбрасвыаем кэш
        if(fflush_unlocked(stream) != 0) {
            stream->_flags |= FSTREAM_ERROR;
            return EOF;
        }
    }

    return c;
}