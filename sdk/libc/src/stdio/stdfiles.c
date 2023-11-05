#include "stdio.h"
#include "stdio_impl.h"

static char stdout_buffer[STDIO_BUFFER_LENGTH];
static char stdin_buffer[STDIO_BUFFER_LENGTH];

static FILE __stderr = {
    ._flags = FSTREAM_UNBUFFERED | FSTREAM_CANWRITE,
    ._fileno = 2,
    ._buffer = NULL
};

static FILE __stdout = {
    ._flags = FSTREAM_CANWRITE,
    ._fileno = 1,
    ._buffer = stdout_buffer,
    ._buf_len = STDIO_BUFFER_LENGTH
};

static FILE __stdin = {
    ._flags = FSTREAM_CANREAD,
    ._fileno = 0,
    ._buffer = stdin_buffer,
    ._buf_len = STDIO_BUFFER_LENGTH
};

FILE *stderr = &__stderr;
FILE *stdout = &__stdout;
FILE *stdin = &__stdin;