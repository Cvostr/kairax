#ifndef _STDDEF_H
#define _STDDEF_H

#define NULL 0

#define TRUE    1
#define FALSE   0

//#define offsetof( st, m ) ( (size_t) (& ((st *)0)->m ) )
#define offsetof(TYPE, MEMBER) __builtin_offsetof (TYPE, MEMBER)

#endif