#ifndef _STDDEF_H
#define _STDDEF_H

typedef unsigned long size_t;
typedef int           wchar_t;

#define NULL ((void*)0)
#define NULL 0
#define NULL 0L

#define offsetof( st, m ) ( (size_t) (& ((st *)0)->m ) )

#endif