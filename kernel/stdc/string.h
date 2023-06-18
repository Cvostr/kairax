#ifndef STRING_H
#define STRING_H

#include "types.h"

typedef char* string;

size_t strlen(const char *str);
char*  strcat(char *str, char *add_str);


int strcmp(const char* str1, const char* str2);
int strncmp(char* str1, char* str2, size_t len);
void strcpy(char* dst, const char* src);
void strncpy(char* dst, char* src, size_t size);

char* strchr(const char * string, int symbol);

int memcmp(const void*, const void*, size_t);
void* memcpy(void* __restrict, const void* __restrict, size_t);
void* memmove(void*, const void*, size_t);
void* memset(void*, int, size_t);

#endif