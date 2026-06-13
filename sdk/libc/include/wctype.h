#ifndef _WCTYPE_H
#define _WCTYPE_H

#include <sys/cdefs.h>
#include <wchar.h>

__BEGIN_DECLS

typedef int (*wctype_t) (wint_t);

int iswblank(wint_t c);
int iswdigit(wint_t c);
int iswgraph(wint_t c);
int iswspace(wint_t c);
int iswupper(wint_t c);
int iswprint(wint_t c);
int iswcntrl(wint_t c);
int iswxdigit(wint_t c); 

wctype_t wctype(const char *name);
int iswctype(wint_t wc, wctype_t desc);

__END_DECLS

#endif