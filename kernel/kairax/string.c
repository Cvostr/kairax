#include "string.h"
#include "mem/kheap.h"
#include "kstdlib.h"

size_t strlen(const char* str)
{
	char* begin = (char*) str;

	if (begin == NULL) {
        return 0;
    }

  	while (*str++);
	
  	return str - begin - 1;
}

char* strcat(char *str, char* add_str)
{
  	size_t i,j;

  	for (i = 0; str[i] != '\0'; i++);
  		for (j = 0; add_str[j] != '\0'; j++)
    		str[i+j] = add_str[j];

  	str[i+j] = '\0';

  	return str;
}

char* strncat(char *str, const char *add_str, size_t num)
{
	size_t i,j;

	for (i = 0; str[i] != '\0'; i++);
		for (j = 0; add_str[j] != '\0' && j < num; j++)
		  str[i+j] = add_str[j];

	str[i+j] = '\0';

	return str;
}

int memcmp(const void* aptr, const void* bptr, size_t size) {
	const unsigned char* a = (const unsigned char*) aptr;
	const unsigned char* b = (const unsigned char*) bptr;
	for (size_t i = 0; i < size; i++) {
		if (a[i] < b[i])
			return -1;
		else if (b[i] < a[i])
			return 1;
	}
	return 0;
}

void* memset(void* bufptr, int value, size_t size) {
	unsigned char* buf = (unsigned char*) bufptr;
	for (size_t i = 0; i < size; i++)
		buf[i] = (unsigned char) value;
	return bufptr;
}

void* memcpy(void* restrict dstptr, const void* restrict srcptr, size_t size) {
	unsigned char* dst = (unsigned char*) dstptr;
	const unsigned char* src = (const unsigned char*) srcptr;
	for (size_t i = 0; i < size; i++)
		dst[i] = src[i];
	return dstptr;
}

void* memmove(void* dstptr, const void* srcptr, size_t size) {
	unsigned char* dst = (unsigned char*) dstptr;
	const unsigned char* src = (const unsigned char*) srcptr;
	if (dst < src) {
		for (size_t i = 0; i < size; i++)
			dst[i] = src[i];
	} else {
		for (size_t i = size; i != 0; i--)
			dst[i-1] = src[i-1];
	}
	return dstptr;
}

int strcmp(const char* str1, const char* str2){
	if(strlen(str1) != strlen(str2))
		return 1;

  	for(int i = 0; i < strlen(str1); i ++){
    		if(str1[i] != str2[i]) 
			return 1;
  	}
  	return 0;
}

int strncmp(const char* str1, const char* str2, size_t len)
{
  	for(int i = 0; i < len; i ++)
	{
    	if(str1[i] != str2[i]) 
			return 1;
  	}
  	return 0;
}

void strcpy(char* dst, const char* src)
{
	size_t len = strlen(src);

  	for (uint32_t it = 0; it < len; it ++){
    	*(dst++) = src[it];
  	}

	*(dst++) = '\0';
}

void strncpy(char* dst, const char* src, size_t size)
{
	size_t len = strlen(src);
	len = MIN(len, size);
	for (uint32_t it = 0; it < len; it ++){
    	*(dst++) = src[it];
  	}

	*(dst++) = '\0';
}

char* strchr(const char * string, int symbol)
{
	size_t len = strlen(string);

	for(size_t i = 0; i < len; i ++){
		if(string[i] == symbol)
			return (char*)string + i;
	}

	return NULL;
}

char* strrchr(const char* string, int symbol)
{
	char* s = (char*) string;
	char* result = 0;

  	do {
    	if (*s == symbol)
      		result = (char*) s;

  	} while (*s++);
  	
	return (result);
}

char *strdup (const char *__s)
{
    size_t strsize = strlen(__s);
	if (strsize == 0) {
		return NULL;
	}
	
    char* str = kmalloc(strsize + 1);
    str[strsize] = '\0';

    if (str == NULL) {
        return NULL;
    }

    return memcpy(str, __s, strsize);
}