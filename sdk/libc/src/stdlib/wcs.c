#include "stdlib.h"
#include "wchar.h"
#include "string.h"

int wctomb(char *s, wchar_t wc)
{
    return wcrtomb(s, wc, NULL);
}

int mbtowc(wchar_t *pwc, const char *s, size_t n)
{
    return mbrtowc(pwc, s, n, NULL);
}

size_t wcstombs(char *dest, const wchar_t *src, size_t dsize)
{
    const wchar_t** tmp = &src;
    return wcsrtombs(dest, tmp, dsize, NULL);
}

size_t mbstowcs(wchar_t *dest, const char *src, size_t dsize)
{
    mbstate_t state;
    const char **tmp = &src;
    
    memset(&state, 0, sizeof(state));
    return mbsrtowcs(dest, tmp, dsize, &state);
}