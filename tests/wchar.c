#include "wchar.h"
#include "stdio.h"
#include <wctype.h>
#include "stdlib.h"

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


    char mbsb[5];
    mbsb[4] = 0;
    size_t len = wctomb(mbsb, L'F');
    printf("F=%s\n", mbsb);
    len = wctomb(mbsb, L'П');
    printf("П=%s\n", mbsb);

    char wcbuf[60];
    wcstombs(wcbuf, L"Привет, мир!", sizeof(wcbuf));
    printf("Converted to UTF-8 Russian string: %s\n", wcbuf);
    len = wcstombs(wcbuf, L"Привет, мир! АБВГДЕЗЗЫ", 11);
    printf("Converted to UTF-8 Russian truncated string: %s, its len %i\n", wcbuf, len);
    len = wcstombs(NULL, L"Привет, мир!", 10);
    printf("wcstombs with NULL dest. result len %i\n", len);


    return 0;
}