#include "stdlib.h"
#include "ctype.h"

long int strtol(const char *nptr, char **endptr, int base)
{
    char *curpos = (char*) nptr;
    int neg = 0;

    while (isspace(*curpos))
        curpos++;

    switch (*curpos) {
        case '-': 
            neg = 1;
        case '+':
            curpos++;
    }

    long int val = strtoul(curpos, endptr, base);
    
    return neg ? -val : val;
}

unsigned long int strtoul(const char *nptr, char **endptr, int base)
{
    unsigned long int val = 0;
    char *curpos = (char*) nptr;

    while (isspace(*curpos))
        curpos++;

    while (isdigit(*curpos)) {
        val = val * 10 + (*curpos++ - '0');
    }

    if (*endptr != NULL) {
        *endptr = curpos;
    }

    return val;
}