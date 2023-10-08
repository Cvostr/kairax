#ifndef _LIMITS_H_
#define _LIMITS_H_

#if '\xff' > 0
#define CHAR_MIN 0
#define CHAR_MAX 255
#else
#define CHAR_MIN (-128)
#define CHAR_MAX 127
#endif

#define SHRT_MAX    0x7fff
#define SHRT_MIN	(-1 - SHRT_MAX)
#define USHRT_MAX	(SHRT_MAX * 2 + 1)

#define INT_MAX	    0x7fffffff
#define INT_MIN     (-1 - INT_MAX)
#define UINT_MAX	(INT_MAX * 2u + 1)

#define LONG_MAX    0x7fffffffffffffffl
#define LONG_MIN	(-1l - LONG_MAX)
#define ULONG_MAX	(LONG_MAX * 2ul + 1)

#define LLONG_MAX	LONG_MAX
#define LLONG_MIN	(-1ll - LLONG_MAX)

#endif