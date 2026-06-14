#include "wctype.h"
#include "string.h"

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

int iswgraph(wint_t c)
{
    return c >= 0x21 && c <= 0x7E;
}

int iswxdigit(wint_t c)
{
    return iswdigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

int iswprint(wint_t c)
{
    // TODO: уточнить
	return c >= 0x20 && c <= 0x7E;
}

wint_t towlower(wint_t c)
{
    if ('A' <= c && c <= 'Z')
    	c += 'a' - 'A';
  	return c;
}

wint_t towupper(wint_t c)
{
	if ('a' <= c && c <= 'z')
    	c += 'A' - 'a';
  	return c;
}

struct { 
    const char* name;
    wctype_t func;
} types[] = 
{
    { "blank", iswblank },
    { "cntrl", iswcntrl },
    { "digit", iswdigit },
    { "graph", iswgraph },
    { "print", iswprint },
    { "space", iswspace },
    { "upper", iswupper },
    { "xdigit", iswxdigit },
};

wctype_t wctype(const char* name)
{
    size_t elements = sizeof(types) / sizeof(types[0]);

    for (size_t i = 0; i < elements; i++)
    {
        if (strcmp(name, types[i].name) == 0)
        {
            return types[i].func;
        }
    }

    return (wctype_t)0;
}

int iswctype(wint_t wc, wctype_t desc)
{
    return desc(wc);
}