#include "stdio.h"
#include "stdio_impl.h"
#include "string.h"
#include "stdlib.h"

#define __MIN(a,b) (((a)<(b))?(a):(b))

size_t skip_to_percent(const char* format) {
    size_t n = 0;
    while (format[n] != '\0' && format[n] != '%') {
        n++;
    }
    return n;
}

int printf_write_padding(struct arg_printf* fn, char chr, size_t len) 
{
    if (len <= 0)
        return 0;

    for (size_t i = 0; i < len; i ++) {
        fn->put(fn->data, &chr, 1);
    }

    return len;
}

int write_padded(struct arg_printf* fn, int pad_left, const char* str, char ch, ssize_t len) 
{   
    int written = 0;
    size_t strl = strlen(str);

    if (!pad_left && len > strl) {
        written += printf_write_padding(fn, ch, len - strl);
    }

    fn->put(fn->data, str, strl);
    written += strl;
    
    if (pad_left && len > strl) {
        written += printf_write_padding(fn, ch, len - strl);
    }

    return written;
}

int printf_generic(struct arg_printf* fn, const char *format, va_list args)
{
    size_t len;
    int written = 0;
    char ch;
    char* str;
    double dval;
    int sign; // число со знаком
    int base; // система счисления
    int longnum;
    int uppercase = 0;
    char buf[128];
    long long llvalue;
    int pad_left = 0;
    int width = 0;
    int precision_flag = 0;
    int precision;
    char pad_char = ' ';
    int capitalize = 0;

    while (*format) {

        len = skip_to_percent(format);
        fn->put(fn->data, format, len);
        written += len;
        format += len;

        if (*format == '%') {
            format++;

            longnum = 0;
            sign = 0;
            uppercase = 0;
            pad_left = 0;
            width = 0;
            pad_char = ' ';
            capitalize = 0;
            precision_flag = 0;
            precision = 0;

printf_nextchar:
            switch (ch = *(format++)) {
                case 'c':
                    ch = (char) va_arg(args, int);
                case '%':
                    fn->put(fn->data, &ch, 1);
                    written ++;
                    break;
                case 's':
                    str = va_arg(args, char*);
                    if (precision_flag == 1)
                    {
                        len = __MIN(strlen(str), precision);
                        fn->put(fn->data, str, len);
                        written += len;
                        break;
                    }
                    written += write_padded(fn, pad_left, str, pad_char, width);
                    break;
                case 'z':
                case 'l':
                    longnum++;
                    goto printf_nextchar;
                case '-':
                    pad_left = 1;
                    goto printf_nextchar;
                case '.':
                    // todo : добавить точность
                    pad_char = '0';
                    // Флаг того, что устанавливается точность/максимальная длина
                    precision_flag = 1;
                    goto printf_nextchar;
                case '*':
                    if (precision_flag == 0)
                        width = va_arg(args, int);
                    else
                        precision = va_arg(args, int);
                    goto printf_nextchar;
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                    if (precision_flag == 0)
                    {
                        if (width == 0 && ch == '0') {
                            pad_char = '0';
                        } else {
                            width = width * 10 + (ch - '0');
                        }
                    }
                    else
                    {
                        precision = precision * 10 + (ch - '0');
                    }
                    goto printf_nextchar;

                case 'b':
                    base = 2;
                    goto printf_numeric;
                case 'p':
                    pad_char = '0';
                    longnum = 2;
                    width = 16;
                case 'X':
                    uppercase = 1;
                case 'x':
                    base = 16;
                    goto printf_numeric;
                
                case 'd':
                case 'i':
                    sign = 1;
                case 'u':
                    base = 10;
                    goto printf_numeric;
                case 'o':
                    base = 8;
printf_numeric:
                    if (longnum > 1) {
                        llvalue = va_arg(args, long long);
                    } else if (longnum == 1) {
                        llvalue = va_arg(args, long);
                    } else {
                        llvalue = va_arg(args, int);

                        if (sizeof(int) != sizeof(long) && sign == 0) {
                            llvalue &= ((unsigned int)-1);
                        }
                    }

                    if (longnum > 1) {
                        lltoa(llvalue, buf, base);
                    } else {
                        ltoa(llvalue, buf, base);
                    }

                    if (precision_flag == 1)
                        width = precision;

                    // Записать
                    written += write_padded(fn, pad_left, buf, pad_char, width);
                    break;
                case 'G':
                    capitalize = 1;
                case 'f':
                case 'F':
                case 'g':
                    if (precision_flag == 0)
                        precision = 6;
                    dval = va_arg(args, double);
                    __dtostr(dval, buf, sizeof(buf), width, precision, capitalize ? 0x02 : 0);
                    fn->put(fn->data, buf, strlen(buf));
                    written += strlen(buf);

                    break;
            }
        }
    }

    return written;
}