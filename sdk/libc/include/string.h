#ifndef _STRING_H
#define _STRING_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stddef.h"

/* Copy N bytes of SRC to DEST.  */
extern void *memcpy (void *__restrict __dest, const void *__restrict __src, size_t __n);

/* Copy N bytes of SRC to DEST, guaranteeing
   correct behavior for overlapping strings.  */
extern void *memmove (void *__dest, const void *__src, size_t __n);

/* Set N bytes of S to C.  */
extern void *memset (void *__s, int __c, size_t __n);

/* Compare N bytes of S1 and S2.  */
extern int memcmp (const void *__s1, const void *__s2, size_t __n);

/* Find the length of STRING, but scan at most MAXLEN characters.
   If no '\0' terminator is found in that many characters, return MAXLEN.  */
extern size_t strnlen (const char *__string, size_t __maxlen);

/* Return the length of S.  */
extern size_t strlen (const char *__s);

/* Copy SRC to DEST.  */
extern char *strcpy (char *__restrict __dest, const char *__restrict __src);

/* Copy no more than N characters of SRC to DEST.  */
extern char *strncpy (char *__restrict __dest, const char *__restrict __src, size_t __n);

/* Append SRC onto DEST.  */
extern char *strcat (char *__restrict __dest, const char *__restrict __src);

/* Append no more than N characters from SRC onto DEST.  */
extern char *strncat (char *__restrict __dest, const char *__restrict __src, size_t __n);

/* Compare S1 and S2.  */
extern int strcmp (const char *__s1, const char *__s2);

/* Compare N characters of S1 and S2.  */
extern int strncmp (const char *__s1, const char *__s2, size_t __n);

/* Duplicate S, returning an identical malloc'd string.  */
extern char *strdup (const char *__s);

/* Return a malloc'd copy of at most N bytes of STRING.  The
   resultant string is terminated even if no null terminator
   appears before STRING[N].  */
extern char *strndup (const char *__string, size_t __n);

/* Find the first occurrence of C in S.  */
extern char *strchr (const char *__s, int __c);

extern char *strrchr(const char* __s, int __c);

extern char *strstr(const char* __s, const char* __n);

extern char* strrev(char *str);

#ifdef __cplusplus
}
#endif

#endif