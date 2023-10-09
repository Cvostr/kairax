#include "stdio.h"
#include "stdio_impl.h"
#include "string.h"
#include "stdlib.h"

size_t skip_to_percent(const char* format) {
    size_t n = 0;
    while (format[n] != '\0' && format[n] != '%') {
        n++;
    }
    return n;
}

int printf_generic(struct arg_printf* fn, const char *format, va_list arg_ptr)
{
    size_t len;
    char ch;
    char* str;
    int sign = 0; // число со знаком
    int base; // система счисления
    int longnum = 0;
    char buf[128];
    long long llvalue;

    while (*format) {

        len = skip_to_percent(format);
        fn->put(fn->data, format, len);
        format += (len);

        if (*format == '%') {
            format++;
            switch (ch = *(format++)) {
                case 'c':
                    ch = (char) va_arg(arg_ptr, int);
                case '%':
                    fn->put(fn->data, &ch, 1);
                    break;
                case 's':
                    str = va_arg(arg_ptr, char*);
                    fn->put(fn->data, str, strlen(str));
                    break;
                case 'l':
                    longnum++;
                    continue;
                case 'b':
                    base = 2;
                case 'x':
                    base = 16;
                case 'd':
                case 'i':
                    sign = 1;
                case 'u':
                    base = 10;

                    if (longnum > 1) {
                        llvalue = va_arg(arg_ptr, long long);
                    } else if (longnum == 1) {
                        llvalue = va_arg(arg_ptr, long);
                    } else {
                        llvalue = va_arg(arg_ptr, int);

                        if (sizeof(int) != sizeof(long) && sign == 0) {
                            llvalue &= ((unsigned int)-1);
                        }
                    }

                    if (longnum > 1) {
                        lltoa(llvalue, buf, base);
                    } else {
                        ltoa(llvalue, buf, base);
                    }

                    fn->put(fn->data, buf, strlen(buf));
                    sign = 0;
                    break;
            }
        }
    }
}