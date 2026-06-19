#include "wchar.h"
#include "stdio.h"
#include <wctype.h>
#include "stdlib.h"
#include "string.h"

int main(int argc, char **argv) 
{
    size_t i = 0;
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
    memset(mbsb, 0, sizeof(mbsb));
    size_t len = wctomb(mbsb, L'F');
    printf("F=%s, len=%i\n", mbsb, len);
    len = wctomb(mbsb, L'П');
    printf("П=%s, len=%i\n", mbsb, len);

    char wcbuf[60];
    wcstombs(wcbuf, L"Привет, мир!", sizeof(wcbuf));
    printf("Converted to UTF-8 Russian string: %s\n", wcbuf);
    len = wcstombs(wcbuf, L"Привет, мир! АБВГДЕЗЗЫ", 11);
    printf("Converted to UTF-8 Russian truncated string: %s, its len %i\n", wcbuf, len);
    len = wcstombs(NULL, L"Привет, мир!", 10);
    printf("wcstombs with NULL dest. result len %i\n", len);

    wchar_t wcsbuf[30];
    mbstate_t mbstate;
    memset(&mbstate, 0, sizeof(mbstate));
    printf("mbsinit() after clear %i\n", mbsinit(&mbstate));

    // 
    memset(wcbuf, 0, sizeof(wcbuf));
    len = mbrtowc(wcsbuf, "П", 3, &mbstate);
    wctomb(wcbuf, wcsbuf[0]);
    printf("П after mbrtowc(%i) + wctomb %s\n", len, wcbuf);

    len = mbrtowc(wcsbuf, "F", 2, &mbstate);
    printf("F after mbrtowc(%i) = %c\n", len, wcsbuf[0]);

    char P_str[] = "П";
    memset(wcbuf, 0, sizeof(wcbuf));
    for (i = 0; i < 4; i ++)
    {
        len = mbrtowc(wcsbuf, P_str + i, 1, &mbstate);
        wctomb(wcbuf, wcsbuf[0]);
        printf("TRY %i, mbrtowc = %i\n", i, len);

        if (len == (size_t) -2)
            continue;
        else
            break;
    }
    printf("П after mbrtowc(%i) + wctomb %s\n", len, wcbuf);

    char mbsstring[] = "Привет, мир! 123";
    mbstowcs(wcsbuf, mbsstring, sizeof(wcsbuf));
    wcstombs(wcbuf, wcsbuf, sizeof(wcbuf));
    printf("orig Str %s, after mbstowcs + wcstombs '%s'\n", mbsstring, wcbuf);

    return 0;
}