#include "wctype.h"

int iswblank(wint_t c)
{
    return c == ' ' || c == '\t';
}

int iswdigit(wint_t c)
{
    return ((unsigned int) c - '0') < 10;
}

int iswspace(wint_t c)
{
    return c == ' ' || c == '\n' || c == '\r' || c == '\v' || c == '\t' || c == '\f';
}

int iswupper(wint_t c)
{
    return (c >= 'A' && c <= 'Z');
}

int iswcntrl(wint_t c)
{
    return ((unsigned int)c) < 32u || c == 127;
}

int iswprint(wint_t c)
{
    // TODO: уточнить
	return c >= 0x20 && c <= 0x7E;
}