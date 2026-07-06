#include "stdio.h"
#include "stdio_impl.h"

int ungetc(int c, FILE *stream)
{
    int tmp;
    mtx_lock(&stream->_lock);
    tmp = ungetc_unlocked(c, stream);
    mtx_unlock(&stream->_lock);
    return tmp;
}

int ungetc_unlocked(int c, FILE *stream)
{
    if (c == EOF)
        return EOF;

    if (stream->_ungotten >= UNGETC_BUFSZ)
        return EOF;

    stream->_ungetbuf[stream->_ungotten ++] = (unsigned char) c;
    
    // снимаем EOF, если был 
    stream->_flags &= ~(FSTREAM_EOF);
    return c;
}