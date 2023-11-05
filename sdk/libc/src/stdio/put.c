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
    return fwrite(&c, 1, 1, stream);
}