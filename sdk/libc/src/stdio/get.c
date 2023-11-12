#include "stdio.h"
#include "stdio_impl.h"
#include "unistd.h"

int getchar ()
{
    return fgetc(stdin);
}

int fgetc(FILE *f)
{
    if ((f->_flags & FSTREAM_CANREAD) == 0) {
        f->_flags |= FSTREAM_ERROR;
        return EOF;
    }

    if (f->_buf_pos >= f->_buf_size) {
        ssize_t rd = read(f->_fileno, f->_buffer, f->_buf_len);

        if (rd == 0) {
            f->_flags |= FSTREAM_EOF;
            return EOF;
        } else if (rd < 0) {
            f->_flags |= FSTREAM_ERROR;
            return EOF;
        }

        f->_buf_pos = 0;
        f->_buf_size = rd;
    }

    return (unsigned char) f->_buffer[f->_buf_pos ++];
}