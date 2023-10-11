#include "stdio.h"
#include "stdio_impl.h"
#include "ctype.h"

int scanf_generic(struct arg_scanf* fn, const char *format, va_list args)
{
    int spos = 1;
    char ch = 0;
    char sch = 0;

    // получить символ
    sch = fn->getch(fn->data);

    while (*format) {
        ch = *(format++);

        switch (ch) {
            case ' ':
            case '\n':
            case '\t':
            case '\r':
            case '\v':
                while (format && isspace(*format)) {
                    format++;
                }

                while (isspace(sch)) {
                    sch = fn->getch(fn->data);
                    spos++;
                }
                break;
            case '%' : {
                ch = *(format++);

                switch (ch) {
                    case '%':
                        if (ch != sch) {
                            //goto error;
                        }
                        sch = fn->getch(fn->data);
                        spos++;
                        break;
                    case 's':
                        // считываем строку
                        char* dstr = (char *) va_arg(args, char*);
                        while(!isspace(sch)) {
                            sch = fn->getch(fn->data);
                            spos++;
                        }
                        if (sch == EOF)
                            break;
                        
                        while (sch != EOF) {
                            *dstr = sch;
                            dstr++;
                            sch = fn->getch(fn->data);
                            spos++;
                        }
                        break;
                    case 'c':
                        char* dchr = (char *) va_arg(args, char*);
                        *dchr = sch;
                        break;
                }
                break;
            }
        }
    }
}