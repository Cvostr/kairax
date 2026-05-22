#include "wchar.h"
#include "stdio.h"

int main(int argc, char **argv) 
{
    wchar_t *str = L"ABCDПривет!";
    wchar_t *str2 = L"ABCDПривет2!";

    printf("wcslen=%i wcscmp1=%i wcscmp2=%i\n", wcslen(str), wcscmp(str, str), wcscmp(str, str2));
    printf("pos of A=%i pos of П=%i ptr of 6=%p\n", wcschr(str, L'A') - str, wcschr(str, L'П') - str, wcschr(str, L'6'));
    return 0;
}