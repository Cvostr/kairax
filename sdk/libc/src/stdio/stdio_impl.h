#ifndef _STDIO_IMPL_
#define _STDIO_IMPL_

#include "stddef.h"
#include "stdarg.h"
#include "stdint.h"
#include "threads.h"

#define STDIO_BUFFER_LENGTH 2048

#define FSTREAM_INPUT  8
#define FSTREAM_UNBUFFERED 16
#define FSTREAM_CANREAD 128
#define FSTREAM_CANWRITE 256

#define FSTREAM_ERROR 1
#define FSTREAM_EOF 2

struct IO_FILE {
    int         _flags;
    int         _fileno;
    off_t       _off;
    char*       _buffer;
    uint32_t    _buf_pos;   // Позиция в буфере
    uint32_t    _buf_size;  // Количество байт в буфере
    uint32_t    _buf_len;   // Размер выделенного буфера
    struct IO_FILE *_next;	// Связный список для всех FILE. используется в fflush(0)
    mtx_t       _lock;
};

struct arg_printf {
    void *data;
    int (*put)(void*, const void*, size_t);
};

struct arg_scanf {
    void *data;
    int (*getch)(void*);
    int (*putch)(int, void*);
};

struct str_data {
    char* str;
    size_t len;
    size_t size;
};

int printf_generic(struct arg_printf* fn, const char *format, va_list arg_ptr);
int scanf_generic(struct arg_scanf* fn, const char *format, va_list args);

#endif