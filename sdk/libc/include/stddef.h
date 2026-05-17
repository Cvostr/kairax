#ifndef _STDDEF_H
#define _STDDEF_H

typedef unsigned long size_t;
typedef int           wchar_t;

#define NULL ((void*)0)

#define offsetof( st, m ) ( (size_t) (& ((st *)0)->m ) )

// Если wint_t поддерживается GCC по умолчанию, _WINT_T будет определен
// Иначе - определяем сами
#ifndef _WINT_T
#define _WINT_T

#ifndef __WINT_TYPE__
#define __WINT_TYPE__ unsigned int
#endif
typedef __WINT_TYPE__ wint_t;
#endif

#endif