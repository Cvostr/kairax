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