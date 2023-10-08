#include "stdio.h"

static FILE __stderr = {
  ._fileno = 2
};

static FILE __stdout = {
  ._fileno = 1
};

static FILE __stdin = {
  ._fileno = 0
};

FILE *stderr = &__stderr;
FILE *stdout = &__stdout;
FILE *stdin = &__stdin;