#include "stdio.h"
#include "stdio_impl.h"
#include "ctype.h"
#include "stdlib.h"

int scanf_generic(struct arg_scanf* fn, const char *format, va_list args)
{
    int spos = 0;
    char ch = 0; // Текущий символ строки форматирования
    char sch = 0;   // Текущий символ строки 
    int base;
    unsigned long long numval;
    double floatval;
    double factor;
    // Флаги чтения чисел
    int half_count; // Количество флагов - половина
    int long_count; // Количество флагов - длинный
    int flag_signed;  // Число со знаком

    union {
        long long *llv;
        long *lv;
        short *sv;
        int *iv;
        unsigned *uiv;
        char *cv;
    } nv;
    int sign;

    // получить символ
    sch = fn->getch(fn->data);
    spos++;

    while (*format) {
        ch = *(format++);

        switch (ch) {
            case ' ':
            case '\n':
            case '\t':
            case '\r':
            case '\v':
                // Пропустить пробелы строки форматирования
                while (format && isspace(*format)) {
                    format++;
                }
                // Пропустить пробелы входной строки
                while (isspace(sch)) {
                    sch = fn->getch(fn->data);
                    spos++;
                }
                break;
            case '%' : {

                half_count = 0;
                long_count = 0;
                flag_signed = 0;
                sign = 1;            // Изначально положительное число
                numval = 0;

next_modifier:
                ch = *(format++);
                switch (ch) {
                    case 'h':
                        half_count++;
                        goto next_modifier;
                    case 'l':
                        long_count++;
                        goto next_modifier;
                    case '%':
                        if (ch != sch) {
                            //goto error;
                        }
                        sch = fn->getch(fn->data);
                        spos++;
                        break;
                    case 'f':
                        floatval = 0;
                        factor = 1;
                        // Пропускаем пробелы входной строки
                        while(isspace(sch)) {
                            sch = fn->getch(fn->data);
                            spos++;
                        }
                        if (sch == EOF)
                            break;

                        switch (sch) {
                            case '-': 
                                sign = -1;
                            case '+':
                                spos++;
                                sch = fn->getch(fn->data);
                        }

                        // Считываем целую часть
                        while (isdigit(sch)) {
                            floatval = floatval * 10 + (sch++ - '0');
                            // Считать следующий символ
                            sch = fn->getch(fn->data);
                            spos++;
                        }

                        // Добрались до дробной части
                        if (sch == '.') {
                            // Считать следующий символ
                            sch = fn->getch(fn->data);
                            spos++;

                            while (isdigit(sch)) {
                                factor *= 0.1;
                                floatval = floatval + (sch - '0') * factor;
                                // Считать следующий символ
                                sch = fn->getch(fn->data);
                                spos++;
                            }
                        }

                        if (long_count == 0) {
                            *((float *) va_arg(args, float*)) = floatval * sign;
                        } else {
                            *((double *) va_arg(args, double*)) = floatval * sign;
                        }
                        break;
                    case 's':
                        // считываем строку
                        // Указатель - куда писать
                        char* dstr = (char *) va_arg(args, char*);
                        // Пропускаем пробелы входной строки
                        while(isspace(sch)) {
                            sch = fn->getch(fn->data);
                            spos++;
                        }
                        if (sch == EOF)
                            break;

                        // Копируем строку, пока не встречен EOF или пробел
                        while (sch != EOF && !isspace(sch)) {
                            *dstr = sch;
                            dstr++;
                            sch = fn->getch(fn->data);
                            spos++;
                        }
                        // терминируем строку
                        *dstr = '\0';
                        dstr++;
                        break;
                    case 'c':
                        // Пропускаем пробелы входной строки
                        while (isspace(sch)) {
                            sch = fn->getch(fn->data);
                            spos++;
                        }
                        
                        char* dchr = (char *) va_arg(args, char*);
                        *dchr = sch;

                        sch = fn->getch(fn->data);
                        spos++;
                        break;
                    case 'x':
                    case 'X':
                        base = 16;
                        goto scan_number;
                    case 'o':
                        base = 8;
                        goto scan_number;
                    case 'b':
                        base = 2;
                        goto scan_number;
                    case 'i':
                    case 'd':
                        flag_signed = 1;
                    case 'u':
                        base = 10;
scan_number:
                    // Пропустить пробелы
                    while (isspace(sch)) {
                        sch = fn->getch(fn->data);
                        spos++;
                    }

                    // Чтение знака
                    switch (sch) {
                        case '-':
                            sign = -1;
                        case '+':
                            sch = fn->getch(fn->data);
                            spos++;
                    }

                    // Чтение числа
                    while (isalnum(sch)) {
                        int nc = sch & 0xFF;
                        if ((nc >= '0' && nc <= '9')) {
                            nc -= '0';
                        } else if (nc >= 'a' && nc <= 'f') {
                            nc = nc - 'a' + 10;
                        } else if (nc >= 'A' && nc <= 'F') {
                            nc = nc - 'A' + 10;
                        } else {
                            break;
                        }

                        numval = numval * base + nc;

                        // Считать следующий символ
                        sch = fn->getch(fn->data);
                        spos++;
                    }

                    numval *= sign;

                    if (long_count > 1) {
                        if (flag_signed) {
                            nv.llv = (long long *) va_arg(args, long long*);
                            *nv.llv = numval;
                        } else {

                        }
                    } else if (long_count == 1) {
                        if (flag_signed) {
                            nv.lv = (long *) va_arg(args, long*);
                            *nv.lv = numval;
                        } else {

                        }
                    } else if (half_count == 1) {
                        if (flag_signed) {
                            nv.sv = (short *) va_arg(args, short*);
                            *nv.sv = numval;
                        } else {

                        }
                    } else if (half_count > 1) {
                        if (flag_signed) {
                            nv.cv = (char *) va_arg(args, char*);
                            *nv.cv = numval;
                        } else {

                        }
                    } else {
                        if (flag_signed) {
                            nv.iv = (int *) va_arg(args, int*);
                            *nv.iv = numval;
                        } else {
                            nv.uiv = (unsigned *) va_arg(args, unsigned*);
                            *nv.uiv = numval;
                        }
                    }

                }
                break;
            }

            default:
                // Проверка совпадения символов
                //printf("Y %c = %c\n", ch, sch);
                if (ch != sch) {
                    // todo: error
                }

                // Считать следующий символ
                sch = fn->getch(fn->data);
                spos++;
                break;
        }
    }
}