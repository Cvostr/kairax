#include "dirent.h"
#include "stddef.h"
#include "stdlib.h"
#include "string.h"
#include "errno.h"

static void __free_dirarray(int num, struct dirent **namelist)
{
    for (int i = 0; i < num; i ++)
    {
        free(namelist[i]);
    }

    free(namelist);
}

int scandir(const char *dirp,
            struct dirent ***namelist,
            int (*filter)(const struct dirent *),
            int (*compar)(const struct dirent **, const struct dirent **))
{
    int selected = 0;
    DIR *dir;
    struct dirent *currentdir;
    struct dirent *copydir;
    struct dirent **tmparray;

    dir = opendir(dirp);
    if (dir == NULL)
    {
        return -1;
    }

    *namelist = NULL;

    while ((currentdir = readdir(dir)) != NULL)
    {
        if (filter == NULL || filter(currentdir))
        {
            size_t dirent_len = sizeof(struct dirent) + strlen(currentdir->d_name);
            // Расширить имеющийся массив
            tmparray = realloc(*namelist, (selected + 1) * sizeof(struct dirent *));
            if (tmparray == NULL)
            {
                // не получилось перевыделить память - освобождаем по старому указателю 
                __free_dirarray(selected, *namelist);
                closedir(dir);
                errno = -ENOMEM;
                return -1;
            }

            // выделить новый объект dirent
            copydir = malloc(dirent_len);
            if (copydir == NULL)
            {
                // память перевыделена - освобождаем по новому указателю 
                __free_dirarray(selected, tmparray);
                closedir(dir);
                errno = -ENOMEM;
                return -1;
            }

            // копировать текущую временную запись в выходной массив
            memcpy(copydir, currentdir, dirent_len);
            tmparray[selected] = copydir;

            // обновить указатель
            selected ++;
            *namelist = tmparray;
        }
    }

    closedir(dir);

    // отсортировать, если указана функция
    if (compar)
    {
        qsort(*namelist, selected, sizeof(struct dirent *), (int (*) (const void *, const void *)) (compar));
    }

    return selected;
}