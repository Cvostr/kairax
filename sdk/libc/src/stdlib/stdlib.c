#include "stdlib.h"
#include "ctype.h"
#include "unistd.h"
#include "string.h"

int atoi(const char *s)
{
	return atol(s);
}

long atol(const char *s)
{
	return strtol(s, NULL, 10);
}

double atof(const char *nptr)
{
	return strtod(nptr, NULL);
}

char* itoa(int number, char* str, int base)
{
    return ltoa(number, str, base);
}

char* ltoa(long number, char* str, int base)
{
	return lltoa(number, str, base);
}

char* lltoa(long long number, char* str, int base)
{
	int count = 0;
	int neg = 0;

	if (number < 0) {
		neg = 1; 
		number *= -1;
	}

    do {
      	int digit = number % base;
      	str[count++] = (digit > 9) ? digit - 10 + 'A' : digit + '0';
    } while ((number /= base) != 0);
    
	if (neg) {
		str[count++] = '-';
	}
	
    str[count] = '\0';
	strrev(str);

    return str;
}

void __atexit_callall();

void exit(int status)
{
	__atexit_callall();
	_exit(status);
}