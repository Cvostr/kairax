#include "string.h"

extern char* __errors_descriptions[];
extern char* __unknown_error;

extern int max_descs;

char *strerror(int errnum)
{
    char* errdesc = __unknown_error;
    if (errnum < max_descs) {
        errdesc = __errors_descriptions[errnum];
    }

    return errdesc;
}