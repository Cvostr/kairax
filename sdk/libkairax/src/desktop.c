#include "desktop.h"

int foo() {
    return 1;
}

int CreateWindow(int w, int h, const char* title)
{
    return w * h + foo();
}