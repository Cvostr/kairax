#include "stdio.h"
#include "stdio_impl.h"
#include "threads.h"

static char stdout_buffer[STDIO_BUFFER_LENGTH];
static char stdin_buffer[STDIO_BUFFER_LENGTH];

static FILE __stderr = {
    ._flags = FSTREAM_UNBUFFERED | FSTREAM_CANWRITE,
    ._fileno = 2,
    ._buffer = NULL,
    ._next = NULL,
    ._lock = {.type = mtx_plain, .lock = 0}
};

static FILE __stdout = {
    ._flags = FSTREAM_CANWRITE,
    ._fileno = 1,
    ._buffer = stdout_buffer,
    ._buf_len = STDIO_BUFFER_LENGTH,
    ._next = NULL,
    ._lock = {.type = mtx_plain, .lock = 0}
};

static FILE __stdin = {
    ._flags = FSTREAM_CANREAD | FSTREAM_INPUT,
    ._fileno = 0,
    ._buffer = stdin_buffer,
    ._buf_len = STDIO_BUFFER_LENGTH,
    ._next = NULL,
    ._lock = {.type = mtx_plain, .lock = 0}
};

FILE *stderr = &__stderr;
FILE *stdout = &__stdout;
FILE *stdin = &__stdin;