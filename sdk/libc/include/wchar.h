#ifndef _WCHAR_H
#define _WCHAR_H

#include <sys/cdefs.h>
#include <stddef.h>
#include "types/wint_t.h"

__BEGIN_DECLS

#ifndef WCHAR_MIN
#define WCHAR_MIN (-2147483647 - 1)
#endif

#ifndef WCHAR_MAX
#define WCHAR_MAX (2147483647)
#endif

#ifndef WEOF
#define WEOF 0xffffffffu
#endif

typedef struct {
    int __count;
    wchar_t __value;
} mbstate_t;

size_t wcslen(const wchar_t *s);
int wcscmp(const wchar_t *s1, const wchar_t *s2);
wchar_t *wcscpy(wchar_t *dest, const wchar_t *src);
wchar_t* wcschr(const wchar_t *wcs, wchar_t wc);
wchar_t *wcscat(wchar_t *dest, const wchar_t *src);
int wcsncmp(const wchar_t *s1, const wchar_t *s2, size_t n);
wchar_t *wmemcpy(wchar_t *dest, const wchar_t *src, size_t n);
wchar_t *wmemchr(const wchar_t *s, wchar_t c, size_t n);

int mbsinit(const mbstate_t *ps);
size_t wcrtomb(char *s, wchar_t wc, mbstate_t *ps);
size_t mbrtowc(wchar_t *pwc, const char *s, size_t n, mbstate_t *ps);

size_t mbsrtowcs(wchar_t *dest, const char **src, size_t dsize, mbstate_t *ps);
size_t wcsrtombs(char *dest, const wchar_t **src, size_t dsize, mbstate_t *ps);

__END_DECLS

#endif