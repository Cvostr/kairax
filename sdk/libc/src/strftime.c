#include "time.h"
#include "stdio.h"

static const char __short_weekdays [7] [4] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

static const char __weekdays [7] [10] = {
    "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"
};

static const char __short_months [12] [4] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

static const char* __months [12] = {
    "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December"
};

char *__strftime_append(char *dst, char *maxptr, const char *src)
{
    while (*src && dst < maxptr)
		*dst++ = *src++;

    return dst;
}

size_t strftime(char *s, size_t max, const char *format, const struct tm *tm)
{
    if (max == 0)
        return 0;

    char *dst = s;

    while (*format != '\0')
    {
        if (*format == '%')
        {
            format ++;

            // TODO: больше форматов
            switch (*format)
            {
            case '%':
                *s++ = '%';
                break;
            case 'R':
                dst += strftime(dst, s + max - dst, "%H:%M", tm);
                break;
            case 'X':
                dst += strftime(dst, s + max - dst, "%k:%M:%S", tm);
                break;
            case 'D':
                dst += strftime(dst, s + max - dst, "%m/%d/%y", tm);
                break;
            case 'F':
                dst += strftime(dst, s + max - dst, "%Y-%m-%d", tm);
                break;
            case 'T':
                dst += strftime(dst, s + max - dst, "%H:%M:%S", tm);
                break;
            case 'a':
                dst = __strftime_append(dst, s + max, __short_weekdays[tm->tm_wday]);
                break;
            case 'A':
                dst = __strftime_append(dst, s + max, __weekdays[tm->tm_wday]);
                break;
            case 'b':
                dst = __strftime_append(dst, s + max, __short_months[tm->tm_mon]);
                break;
            case 'B':
                dst = __strftime_append(dst, s + max, __months[tm->tm_mon]);
                break;
            case 'Y':
                dst += sprintf(dst, "%04d", tm->tm_year + 1900); break;
            case 'y':
                dst += sprintf(dst, "%02d", tm->tm_year % 100); break;
            case 'm':
                dst += sprintf(dst, "%02d", tm->tm_mon + 1); break;
            case 'M':
                dst += sprintf(dst, "%02d", tm->tm_min); break;
            case 'd':
                dst += sprintf(dst, "%02d", tm->tm_mday); break;
            case 'e':
                dst += sprintf(dst, "%2d", tm->tm_mday); break;
            case 'j':
                dst += sprintf(dst, "%03d", tm->tm_yday); break;
            case 'H':
                dst += sprintf(dst, "%02d", tm->tm_hour); break;
            case 'I':
                dst += sprintf(dst, "%02d", tm->tm_hour % 12); break;
            case 'S':
                dst += sprintf(dst, "%02d", tm->tm_sec); break;
            default:
                break;
            }
        }
        else
        {
            // Обычный символ - просто добавляем
            *dst++ = *format;
        }

        if (dst >= s + max)
	        break;

        format ++;
    }

    return dst - s;
}