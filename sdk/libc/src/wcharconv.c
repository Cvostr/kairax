#include "wchar.h"
#include "errno.h"
#include "string.h"
#include "stdint.h"

static mbstate_t __mbs_internal;

int mbsinit(const mbstate_t *ps)
{
    // намеренно проверяем только по __value, так как mbrtowc опирается на __count
    // и если __count не равен 0, то считает это незавершенной последовательностью
    // и не очищает состояние
    return ps == NULL || ps->__value == 0;
}

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

// https://man7.org/linux/man-pages/man3/mbrtowc.3.html
size_t mbrtowc(wchar_t *pwc, const char *s, size_t n, mbstate_t *ps)
{
    unsigned char c; 
    unsigned char mask;
    unsigned int sequence_len;
    size_t i;

    if (ps == NULL)
    {
        // если не указано, используем общее состояние
        // не потокобезопасно!!!
        ps = &__mbs_internal;
    }

    // third case
    if (s == NULL)
    {
        if (ps->__count > 0)
        {
            errno = EILSEQ;
            return (size_t) -1;
        }
        else
        {
            ps->__count = 0;
            ps->__value = 0;
            return 0;
        }
    }
    
    if (n == 0) 
    {
        // нам нечего дальше делать, если строка не передана
        // считаем это как "неполная строка"
        return (size_t) -2;
    }

    for (i = 0; i < n; i ++)
    {
        c = (unsigned char) s[i];

        // проверим, не является ли символ началом последовательности?
        if ((c & 0xC0) != 0x80) 
        {
            // символ начинает последовательность
            if (ps->__count > 0)
            {
                // если count > 0, при этом началась новая последовательность - значит что то не так
                errno = EILSEQ;
                return (size_t) -1;
            }

            // посчитаем длину последовательности и маску для первого символа
            if ((c & 0x80) == 0) {
                sequence_len = 1;
                mask = 0x7F;
            } else if ((c & 0xE0) == 0xC0) {
                sequence_len = 2;
                mask = 0x3F;
            } else if ((c & 0xF0) == 0xE0) {
                sequence_len = 3;
                mask = 0x0F;
            } else if ((c & 0xF8) == 0xF0) {
                sequence_len = 4;
                mask = 0x07;
            } else {
                //printf("invalid length prefix on byte %i\n", c);
                errno = EILSEQ;
                return (size_t) -1;
            }

            ps->__count = sequence_len;
        }
        else
        {
            // для остальных символов маска одинаковая
            mask = 0x3F;
        }

        // добавить байт к результирующему символу
        ps->__value <<= 6;
        ps->__value |= (c & mask);

        ps->__count--;

        // закончили считывание символа
        if (ps->__count == 0)
            break;
    }

    // Обработали все символы из исходной строки, но последовательность не завершена
    // значит не хватило данных
    if (ps->__count > 0)
    {
        return (size_t) -2;
    }

    if (pwc != NULL)
    {
        *pwc = ps->__value;
    }

    if (ps->__value == 0)
    {
        // если это конец строки, то сбрасываем состояние и выходим
        ps->__count = 0;
        ps->__value = 0;
        return 0;
    }

    ps->__value = 0;
    return i + 1;
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

size_t mbsrtowcs(wchar_t *dest, const char **src, size_t dsize, mbstate_t *ps)
{
    size_t i;
    size_t wclen;

    // если dest - NULL, то ограничения по размеру выходного буфера нет
    if (dest == NULL)
    {
        dsize = UINT64_MAX;
    }

    for (i = 0; i < dsize; i++)
    {
        wclen = mbrtowc(dest == NULL ? NULL : &dest[i], *src, dsize - i, ps);

        if (wclen == 0)
        {
            // встречен '\0' в исходной строке
            break;
        }
        else if (wclen == (size_t) -1)
        {
            // ошибка
            return -1;
        }

        (*src) += wclen;
    }

    return i;
}