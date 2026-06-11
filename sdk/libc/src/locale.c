#include "locale.h"

char *setlocale(int category, const char *locale)
{
    // TODO: Реализовать
    return "C.UTF-8";
}

struct lconv* localeconv(void)
{
    // TODO: Реализовать
    return 0;
}