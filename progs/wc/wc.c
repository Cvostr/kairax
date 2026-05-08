#include "stdio.h"
#include "getopt.h"
#include "unistd.h"
#include "fcntl.h"
#include "string.h"
#include "errno.h"
#include "ctype.h"

int show_lines = 0;
int show_words = 0;
int show_bytes = 0;

int process(FILE *f, const char *fname)
{
    int ch;
    int new_space = 0;

    size_t lines = 0, words = 0, bytes = 0;

    while (!feof(f)) 
    {
		ch = fgetc(f);

        bytes ++;

        if (isspace(ch))
        {
            new_space = 1;
        } 
        else if (new_space == 1)
        {
            words++;
            new_space = 0;
        }
        
        if (ch == '\n') 
        {
            lines++;
        }
    }

    if (show_lines)
        printf("%lu ", lines);
    if (show_words)
        printf("%lu ", words);
    if (show_bytes)
        printf("%lu ", bytes);

    if (strlen(fname) > 0)
        printf("%s\n", fname);
    else
        printf("%c", '\n');
}

int main(int argc, char **argv) 
{
    int opt;
    int rc;
    FILE *file;

    while ((opt = getopt(argc, argv, "lwc")) != -1) {
        switch (opt) {
            case 'l': 
                show_lines = 1;
                break;
            case 'w':
                show_words = 1;
                break;
            case 'c':
                show_bytes = 1;
                break;
        }
    }

    // Если никакие опции не были переданы, показываем сразу все
    if (show_bytes == 0 && show_lines == 0 && show_words == 0)
    {
        show_bytes = 1;
        show_lines = 1;
        show_words = 1;
    }

    if (optind == argc) 
    {
        // файл не задан, принимаем из stdin
        process(stdin, "");
    }
    else
    {
        for (; optind < argc; optind++) 
        {
            char *path = argv[optind];
            file = fopen(path, "rb");
            if (file == NULL)
            {
                printf("wc: %s: %s\n", path, strerror(errno));
                continue;
            }

            process(file, path);
            fclose(file);
        }
    }

    return 0;
}