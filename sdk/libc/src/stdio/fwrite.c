#include "stdio.h"
#include "unistd.h"
#include "stdlib.h"
#include "stdio_impl.h"

size_t fwrite(const void *src, size_t size, size_t count, FILE *f)
{
    size_t tmp;
    
    mtx_lock(&f->_lock);
    tmp = fwrite_unlocked(src, size, count, f);
    mtx_unlock(&f->_lock);
    
    return tmp;
}

size_t fwrite_unlocked(const void *src, size_t size, size_t count, FILE *f)
{
    size_t len = size * count;
    ssize_t res = 0;
    size_t i;

    if ((f->_flags & FSTREAM_CANWRITE) == 0) {
        f->_flags |= FSTREAM_ERROR;
        return 0;
    }

    if (f->_flags & FSTREAM_UNBUFFERED) {
        res = write(f->_fileno, src, len);
        return res / size;
    }
/*
    // Буферизированный поток
    // Вычисляем оставшееся место в буфере
    size_t buffer_space_left = f->_buf_len - f->_buf_pos;
    // Заносим данные в буфер
    for (i = 0; i < buffer_space_left; i ++) {
        f->_buffer[f->_buf_pos ++] = src[i];

        if ( ((unsigned char*)src)[i] == '\n') {
            // Встретился перенос строки - сбрасываем на диск
            if(fflush(f) != 0) {
                f->_flags |= FSTREAM_ERROR;
                return EOF;
            }
        }
    }
*/
    for (i = 0; i < len; i ++) {
        unsigned char c = ((unsigned char*) src) [i];
        if (fputc_unlocked(c, f) == EOF) {
            res = i;
            goto exit;
        }

        res = len;
    }

exit:
    return res / size;
}