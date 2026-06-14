#include "wchar.h"
#include "errno.h"
#include "string.h"

size_t wcrtomb(char *s, wchar_t wc, mbstate_t *ps)
{
    size_t res;
    if (wc <= 0x7F)
    {
        res = 1;
        s[0] = (char) wc;
    }
    else if (wc <= 0x7FF)
    {
        res = 2;
        s[0] = 0xC0 | ((wc >> 6) & 0b11111);
        s[1] = 0x80 | (wc & 0b111111);
    }
    else if (wc <= 0xFFFF)
    {
        res = 3;
        s[0] = 0xE0 | ((wc >> 12) & 0b1111);
        s[1] = 0x80 | ((wc >> 6) & 0b111111);
        s[2] = 0x80 | (wc & 0b111111);
    }
    else if (wc <= 0x10FFFF)
    {
        res = 4;
        s[0] = 0xF0 | ((wc >> 18) & 0b111);
        s[1] = 0x80 | ((wc >> 12) & 0b111111);
        s[2] = 0x80 | ((wc >> 16) & 0b111111);
        s[3] = 0x80 | (wc & 0b111111);
    }
    else
    {
        errno = EILSEQ;
        return (size_t) -1;
    }

    return res;
}

size_t wcsrtombs(char *dest, const wchar_t **src, size_t dsize, mbstate_t *ps)
{
    wchar_t c;
    size_t pos = 0;
    char buf[6];
    size_t written = 0;
    size_t seg_len;
    size_t remain_len = dsize;

    while ((c = **src) != '\0')
    {
        seg_len = wcrtomb(buf, c, ps);
        if (seg_len == (size_t) -1) 
        {
            // errno уже установлено в wcrtomb
            return seg_len;
        }

        if (dest != NULL)
        {
            // проверяем, осталось ли место
            if (remain_len < seg_len) 
            {
                return written;
            }

            // добавляем сегмент к результирующему буферу
            memcpy(dest + written, buf, seg_len);

            // уменьшение счетчика оставшегося места в буфере
            remain_len -= seg_len;
        }

        written += seg_len;
        
        (*src) ++;
    }

    // терминируем результирующую строку, если она есть
    if (dest != NULL)
    {
        dest[written] = '\0';
    }

    return written;
}