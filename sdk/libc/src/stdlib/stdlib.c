#include "stdlib.h"
#include "ctype.h"
#include "unistd.h"
#include "string.h"

int atoi(const char *s)
{
	int n = 0, neg = 0;
	while (isspace(*s)) s++;
	switch (*s) {
	    case '-': neg = 1;
	    case '+': s++;
	}

	while (isdigit(*s))
		n = 10 * n - (*s++ - '0');

	return neg ? n : -n;
}

long atol(const char *s)
{
	int n = 0, neg = 0;
	while (isspace(*s)) s++;
	switch (*s) {
	    case '-': neg = 1;
	    case '+': s++;
	}

	while (isdigit(*s))
		n = 10 * n - (*s++ - '0');

	return neg ? n : -n;
}

double atof(const char *nptr)
{
	return strtod(nptr, NULL);
}

char* itoa(int number, char* str, int base)
{
    int count = 0;

    do {
      	int digit = number % base;
      	str[count++] = (digit > 9) ? digit - 10 + 'A' : digit + '0';
    } while ((number /= base) != 0);
    
    str[count] = '\0';
	strrev(str);

    return str;
}

char* ltoa(long number, char* str, int base)
{
	int count = 0;

    do {
      	int digit = number % base;
      	str[count++] = (digit > 9) ? digit - 10 + 'A' : digit + '0';
    } while ((number /= base) != 0);
    
    str[count] = '\0';
	strrev(str);

    return str;
}

char* lltoa(long long number, char* str, int base)
{
	int count = 0;

    do {
      	int digit = number % base;
      	str[count++] = (digit > 9) ? digit - 10 + 'A' : digit + '0';
    } while ((number /= base) != 0);
    
    str[count] = '\0';
	strrev(str);

    return str;
}

void exit(int status)
{
	_exit(status);
}

int system(const char *command)
{
	// not implemented
	return -1;
}