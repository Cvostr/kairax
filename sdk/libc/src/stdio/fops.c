#include "stdio.h"
#include "unistd.h"
#include "stdlib.h"
#include "stdio_impl.h"

int fclose(FILE *stream)
{
    close(stream->_fileno);
    if (stream->_buffer) {
        free(stream->_buffer);
    }
    free(stream);
}

long ftell(FILE *stream)
{
    return lseek(stream->_fileno, 0, SEEK_CUR);
}

int fseek(FILE *stream, long offset, int whence)
{
    stream->_buf_pos = 0;
    stream->_buf_size = 0;
    return lseek(stream->_fileno, offset, whence);
}

int fflush(FILE *stream)
{
    ssize_t res;

	if (stream->_buf_pos > 0) {
        res = write(stream->_fileno, stream->_buffer, stream->_buf_pos);
        if (res == -1 || res != stream->_buf_pos) {
            stream->_flags = FSTREAM_ERROR;
            return EOF;
        }

        stream->_buf_pos = 0;
    }
    
	return 0;
}