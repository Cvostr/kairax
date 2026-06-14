#include "stdlib.h"
#include "wchar.h"

int wctomb(char *s, wchar_t wc)
{
    return wcrtomb(s, wc, NULL);
}

size_t wcstombs(char *dest, const wchar_t *src, size_t dsize)
{
    const wchar_t** tmp = &src;
    return wcsrtombs(dest, tmp, dsize, NULL);
}