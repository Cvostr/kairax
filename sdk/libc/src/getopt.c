#include "getopt.h"
#include "string.h"
#include "stdio.h"

// https://pubs.opengroup.org/onlinepubs/009696799/functions/getopt.html
int getopt(int argc, char *const argv[], const char *options)
{
    // Для поддержки слитных опций (-abc) 
    static int argoffset;
    static int lastoptind;

    if (optind == 0)
    {
        optind = 1;
        lastoptind = 0;
    }

begin:
    
    char* arg = argv[optind];
    char* optschem;

    // Валидация аргумента
    if (optind > argc || arg == NULL || arg[0] != '-' || arg[1] == 0)
    {
        return -1;
    }

    // Аргумент '--' невалиден
    if (arg[0] == '-' && arg[1] == '-' && arg[2] == 0)
    {
        optind++;
        return -1;
    }

    // Перешли ли мы на новый аргумент?
    if (lastoptind != optind) 
    {
        // Если да, надо сбросить Offset
        lastoptind = optind;
        argoffset = 0;
    }

    // буква параметра
    optopt = arg[argoffset + 1];

    // Если буква - терминатор
    if (optopt == '\0')
    {
        // Значит мы закончили читать этот аргумент и надо переходить к следующему
        optind++;
        goto begin;
    }

    // Ищем опцию в options
    optschem = strchr(options, optopt);
    if (optschem == NULL)
    {
        if (opterr == 1)
            printf("Unknown option `-%c'.\n", optopt);
        optind++;
        return '?';
    }

    if (optschem[1] == ':')
    {
        // Нужен аргумент
        optarg = argv[optind + 1];
        if (optarg == NULL)
        {
            // Аргумента нет
            optind++;

            if (options[0] == ':')
            {
                // A colon ':' shall be returned if getopt() detects a missing argument and the first character of optstring was a colon ':'
                return ':';
            }
            else
            {
                if (opterr == 1)
                    printf("Missing argument for `-%c'.\n", optopt);
                return '?';
            }
        }

        // аргумент найден
        optind += 2;
        return optopt;
    }
    else
    {
        // аргумент не нужен. Увеличиваем смещение в этом же аргументе, вдруг там еще что то есть
        argoffset++;
        return optopt;
    }
}

int getopt_long(int argc, char *const *argv,
	const char *optstring, const struct option *longopts,
	int *longind)
{
    // Для поддержки слитных опций (-abc) 
    static int argoffset;
    static int lastoptind;

    if (optind == 0)
    {
        optind = 1;
        lastoptind = 0;
    }

begin:    
    char* arg = argv[optind];
    char* optschem;

    // Валидация аргумента
    if (optind > argc || arg == NULL || arg[0] != '-' || arg[1] == 0)
    {
        return -1;
    }

    if (arg[1] == '-')
    {
        char *argval = &arg[2];
        // Пробуем искать =
        char *eqpos = strchr(argval, '=');
        // Считаем размер аргумента
        size_t param_sz = eqpos == NULL ? strlen(argval) : eqpos - argval;
        
        struct option* cur = (struct option*) longopts;
        struct option* match = NULL;
        while (cur->name != NULL)
        {
            if ((strncmp(argval, cur->name, param_sz) == 0) && strlen(cur->name) == param_sz)
            {
                match = cur;
                break;
            }
            // Переходим на следующего
            cur += 1;
        }

        if (match != NULL)
        {
            // Нашли аргумент
            if (longind != NULL)
            {
                // Записываем индекс опции
                *longind = match - longopts;
            }

            if (match->has_arg != no_argument)
            {
                if (eqpos != NULL)
                {
                    // Аргумент - все, что после '='
                    optarg = eqpos + 1;
                }
                else
                {
                    // Берем следующий аргумент
                    optarg = argv[optind + 1];
                    // Проверяем, есть ли аргумент
                    if (optarg == NULL && match->has_arg == required_argument)
                    {
                        if (*optstring == ':') 
                            return ':';
                        if (opterr == 1)
                            fprintf(stderr, "argument required: `%s`.\n", match->name);
                        optind++;
                        return '?';
                    }
                    
                    // Увеличиваем индекс - так как аргумент отдельный
                    optind ++;
                }
            }

            // Увеличиваем индекс - пропускаем текущий
            optind ++;
            if (match->flag)
            {
                *(match->flag) = match->val;
            }
            else
            {
                return match->val;
            }

            return 0;
        }
        else
        {
            // не нашли аргумент в списке
            if (*optstring == ':') 
                return ':';
            // вывод ошибки
            if (opterr == 1)
                fprintf(stderr, "invalid option `%.*s`.\n", param_sz, argval);
            ++optind;
            return '?';
        }
    }

    // Далее идет обычная реализация getopt

    // Перешли ли мы на новый аргумент?
    if (lastoptind != optind) 
    {
        // Если да, надо сбросить Offset
        lastoptind = optind;
        argoffset = 0;
    }

    // буква параметра
    optopt = arg[argoffset + 1];

    // Если буква - терминатор
    if (optopt == '\0')
    {
        // Значит мы закончили читать этот аргумент и надо переходить к следующему
        optind++;
        goto begin;
    }

    // Ищем опцию в options
    optschem = strchr(optstring, optopt);
    if (optschem == NULL)
    {
        if (opterr == 1)
            printf("Unknown option `-%c'.\n", optopt);
        optind++;
        return '?';
    }

    if (optschem[1] == ':')
    {
        // Нужен аргумент
        optarg = argv[optind + 1];
        if (optarg == NULL)
        {
            // Аргумента нет
            optind++;

            if (optstring[0] == ':')
            {
                // A colon ':' shall be returned if getopt() detects a missing argument and the first character of optstring was a colon ':'
                return ':';
            }
            else
            {
                if (opterr == 1)
                    printf("Missing argument for `-%c'.\n", optopt);
                return '?';
            }
        }

        // аргумент найден
        optind += 2;
        return optopt;
    }
    else
    {
        // аргумент не нужен. Увеличиваем смещение в этом же аргументе, вдруг там еще что то есть
        argoffset++;
        return optopt;
    }
}