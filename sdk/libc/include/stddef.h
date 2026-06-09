#ifndef _STDDEF_H
#define _STDDEF_H

typedef unsigned long size_t;
typedef signed long ptrdiff_t;
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

// Скорее всего GCC не объявит max_align_t сам, сделаем это за него для C11/C++11 и новее
#if (defined (__STDC_VERSION__) && __STDC_VERSION__ >= 201112L) || (defined(__cplusplus) && __cplusplus >= 201103L)
#ifndef _GCC_MAX_ALIGN_T
#define _GCC_MAX_ALIGN_T
typedef struct 
{
    long long _align_long __attribute__((__aligned__(__alignof__(long long))));
    long double _align_double __attribute__((__aligned__(__alignof__(long double))));
} max_align_t;
#endif
#endif

#endif