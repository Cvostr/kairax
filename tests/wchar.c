#include "wchar.h"
#include "stdio.h"
#include <wctype.h>

int main(int argc, char **argv) 
{
    wchar_t *str = L"ABCDПривет!";
    wchar_t *str2 = L"ABCDПривет2!";

    printf("wcslen=%i wcscmp1=%i wcscmp2=%i\n", wcslen(str), wcscmp(str, str), wcscmp(str, str2));
    printf("pos of A=%i pos of П=%i ptr of 6=%p\n", wcschr(str, L'A') - str, wcschr(str, L'П') - str, wcschr(str, L'6'));


    wchar_t buffer[30];
    wcscpy(buffer, L"Hello ");
    wcscat(buffer, L"World!\n");
    printf("wcscat result %s wcslen %i\n", buffer, wcslen(buffer));

    printf("wc: upper =%p space=%p, ollds=%p\n", wctype("upper"), wctype("space"), wctype("ollds"));
    wctype_t uppert = wctype("upper");
    printf("upper(A)=%i, upper(a)=%i\n", iswctype(L'A', uppert), iswctype(L'b', uppert));

    return 0;
}