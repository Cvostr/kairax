#include "stdlib.h"
#include "ctype.h"

double strtod(const char *nptr, char **endptr)
{
    double val = 0;
    double factor = 1;
    int sign = 1;
    char* curpos = (char*) nptr;

    while (isspace(*curpos))
        curpos++;

    switch (*curpos) {
        case '-': 
            sign = -1;
        case '+':
            curpos++;
    }

    // Считываем целую часть
    while (isdigit(*curpos)) {
        val = val * 10 + (*curpos++ - '0');
    }

    // Добрались до дробной части
    if (*curpos == '.') {
        curpos ++;

        while (isdigit(*curpos)) {
            factor *= 0.1;
            val = val + (*curpos++ - '0') * factor;
        }
    }   

    if (endptr != NULL) {
        *endptr = curpos;
    }

    return val * sign;
}