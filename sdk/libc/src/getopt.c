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

    optschem = strchr(options, optopt);
    if (optschem == NULL)
    {
        if (opterr == 1)
            printf("Unknown option `-%c'.\n", optopt);
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