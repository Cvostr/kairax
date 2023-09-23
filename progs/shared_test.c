#include "string.h"
#include "desktop.h"

char* test_buf[29];

int main(int argc, char** argv) {

    char* test_str = "Hello";
    size_t llen = strlen(test_str);

    memset(test_buf, 0, 29);

    char* found = strchr(test_str, 'l');

    int rc = strcmp(test_str, "Hello");

    rc = CreateWindow(12, 12, NULL);

    return 0;
}