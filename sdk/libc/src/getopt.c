#include "getopt.h"
#include "string.h"
#include "stdio.h"

// https://pubs.opengroup.org/onlinepubs/009696799/functions/getopt.html
int getopt(int argc, char *const argv[], const char *options)
{
    if (optind == 0)
        optind = 1;
    
    char* arg = argv[optind];
    char* optschem;

    if (optind > argc || arg == NULL || arg[0] != '-' || arg[1] == 0)
    {
        return -1;
    }

    if (arg[0] == '-' && arg[1] == '-' && arg[2] == 0)
    {
        optind++;
        return -1;
    }

    // буква параметра
    optopt = arg[1];

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
        // аргумент не нужен
        optind++;
        return optopt;
    }
}