#ifndef STRING_H
#define STRING_H

#include "types.h"

typedef char* string;

size_t strlen(const char *str);
char*  strcat(char *str, char *add_str);
char*  strncat(char *str, const char *add_str, size_t num);

int strcmp(const char* str1, const char* str2);
int strncmp(const char* str1, const char* str2, size_t len);
void strcpy(char* dst, const char* src);
void strncpy(char* dst, const char* src, size_t size);

char* strchr(const char * string, int symbol);
char* strrchr(const char * string, int symbol);
char* strrnchr(const char * string, int symbol);

char *strdup (const char *__s);

int memcmp(const void*, const void*, size_t);
void* memcpy(void* __restrict, const void* __restrict, size_t);
void* memmove(void*, const void*, size_t);
void* memset(void*, int, size_t);

#endif