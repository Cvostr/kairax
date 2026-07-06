#include "stdio.h"
#include "stdio_impl.h"
#include "stdlib.h"
#include "errno.h"

int setvbuf_unlocked(FILE *stream, char *buf, int mode, size_t size)
{   
    if (mode != _IONBF && mode != _IOLBF && mode != _IOFBF)
    {
        errno = EINVAL;
        return -1;
    }

    if (buf)
    {
        if (stream->_buffer != NULL && (stream->_flags & FSTREAM_STATICBUF) == 0) 
        {
            free(stream->_buffer);
        } 

        stream->_buffer = buf;
        stream->_flags &= ~(FSTREAM_STATICBUF);
    }
    else
    {
        if (size == 0) {
            // по доке, если size = 0, то делается отложенное выделение
            // но чтобы не заебываться, мы выделим 4кб прямо тут 
            size = 4096;
        }

        char *newbuf = malloc(size);
        if (newbuf == NULL)
            return -1;

        if (stream->_buffer != NULL && (stream->_flags & FSTREAM_STATICBUF) == 0) 
        {
            free(stream->_buffer);
        }

        stream->_buffer = newbuf;
        stream->_flags &= ~(FSTREAM_STATICBUF);
    }

    stream->_ungotten = 0;
    stream->_buf_pos = 0;
    stream->_buf_size = 0;
    stream->_buf_len = size;

    switch (mode)
    {
    case _IONBF:
        // No Buffer
        stream->_flags |= FSTREAM_UNBUFFERED;
        stream->_flags &= ~(FSTREAM_LINEBUF);
        break;
    case _IOLBF:
        // Line Buffer
        stream->_flags &= ~(FSTREAM_UNBUFFERED);
        stream->_flags |= FSTREAM_LINEBUF;
        break;
    case _IOFBF:
        // Full Buffer
        stream->_flags &= ~(FSTREAM_UNBUFFERED | FSTREAM_LINEBUF);
        break;
    default:
        return -1;
    }

    return 0;
}

int setvbuf(FILE *stream, char *buf, int mode, size_t size)
{
    int tmp;
    mtx_lock(&stream->_lock);
    tmp = setvbuf_unlocked(stream, buf, mode, size);
    mtx_unlock(&stream->_lock);
    return tmp;
}