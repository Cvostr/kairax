#include "stdio.h"
#include "unistd.h"
#include "stdlib.h"
#include "stdio_impl.h"

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