#ifndef _WCTYPE_H
#define _WCTYPE_H

#include <sys/cdefs.h>
#include <wchar.h>

__BEGIN_DECLS

int iswblank(wint_t c);
int iswdigit(wint_t c);
int iswspace(wint_t c);
int iswupper(wint_t c);
int iswprint(wint_t c);
int iswcntrl(wint_t c);

__END_DECLS

#endif