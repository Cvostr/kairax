#include "string.h"

static char *__strtok_pos;

char *strtok(char *s, const char *delim)
{
    return strtok_r(s, delim, &__strtok_pos);
}

char *strtok_r(char *s, const char *delim, char **ptrptr)
{
    char* result = NULL;
    if (s == NULL)
    {
        s = *ptrptr;
    }

    // пропустить символы вначале
    s += strspn(s, delim);

    if (*s)
    {
        result = s;
        // ищем следующее вхождение
        s += strcspn(s, delim);

        if (*s) {
            *s = '\0';
            s++;
        }
    }

    *ptrptr = s;

    return result;
}