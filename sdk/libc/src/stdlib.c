#include "stdlib.h"
#include "ctype.h"

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

char* itoa(int number, char* str, int base)
{
    int count = 0;

    do {
      	int digit = number % base;
      	str[count++] = (digit > 9) ? digit - 10 + 'A' : digit + '0';
    } while ((number /= base) != 0);
    
    str[count] = '\0';
    int i;
    
    for (i = 0; i < count / 2; ++i) {
      	char symbol = str[i];
      	str[i] = str[count - i - 1];
      	str[count - i - 1] = symbol;
    }

    return str;
}

int abs (int i)
{
    return i < 0 ? -i : i;
}

long int labs (long int i)
{
    return i < 0 ? -i : i;
}

long long int llabs (long long int i)
{
    return i < 0 ? -i : i;
}