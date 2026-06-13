#ifndef _WINT_T_DEFINED
#define _WINT_T_DEFINED

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