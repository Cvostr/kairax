#include "string.h"
#include "errors.h"

int __utf32_char_to_utf8(char *s, uint32_t wc, size_t *size)
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
        return -EILSEQ;
    }

    *size = res;
    return 0;
}

int str_utf16_to_utf8(char *dest, const uint16_t *src, size_t isize, size_t dsize, size_t *written)
{
    uint16_t c;
    size_t pos = 0;
    char buf[6];
    size_t _written = 0;
    size_t seg_len;
    size_t remain_len = dsize;
    int rc;

    while (((c = src[pos]) != '\0') && (pos < isize))
    {
        if ((rc = __utf32_char_to_utf8(buf, c, &seg_len)) != 0) 
            return rc;

        if (dest != NULL)
        {
            // проверяем, осталось ли место
            if (remain_len < seg_len) 
            {
                *written = _written;
                return 0;
            }

            // добавляем сегмент к результирующему буферу
            memcpy(dest + _written, buf, seg_len);

            // уменьшение счетчика оставшегося места в буфере
            remain_len -= seg_len;
        }

        _written += seg_len;
        pos++;
    }

    // терминируем результирующую строку, если она есть
    if (dest != NULL)
    {
        dest[_written] = '\0';
    }

    *written = _written;
    return 0;
}