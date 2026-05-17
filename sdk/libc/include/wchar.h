#ifndef _WCHAR_H
#define _WCHAR_H

#include <sys/cdefs.h>

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

__END_DECLS

#endif