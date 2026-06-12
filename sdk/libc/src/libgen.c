#include "libgen.h"
#include "string.h"

static char *dot_sym = ".";

// https://pubs.opengroup.org/onlinepubs/7908799/xsh/basename.html
char *basename(char *path)
{
    char *slashpos;
    if (path == NULL || *path == 0)
        return dot_sym;

    while (1)
    {
        // найдем позицию последнего /
        slashpos = strrchr(path, '/');
        // символа нет - значит строка состояла только из имени файла
        if (slashpos == NULL)
            return path;
        // строка заканчивается символом / (например /proc/1/)
        if (slashpos[1] == 0)
        {
            // строка - просто /
            if (path == slashpos)
                return path;

            // убираем последний / и прогоняем алгоритм заново
            *slashpos = 0; 
            continue;
        }

        return slashpos + 1;
    }

    return 0;
}

// https://pubs.opengroup.org/onlinepubs/009695399/functions/dirname.html
char *dirname(char *path)
{
    char *slashpos;
    if (path == NULL || *path == 0)
        return dot_sym;

    while (1)
    {
        // найдем позицию последнего /
        slashpos = strrchr(path, '/');

        // символа / нет - значит строка состояла только из имени файла
        if (slashpos == NULL)
            return dot_sym;

        // строка заканчивается символом / (например /usr/bin/, или /usr/bin////)
        if (slashpos[1] == 0 && slashpos > path)
        {
            // удаляем слэши справа
            while (*slashpos == '/' && slashpos > path)
            {
                *slashpos = '\0';
                slashpos --;
            }

            // перезапускаем алгоритм
            continue;
        }

        if (slashpos != path)
        {
            // если позиция слэша не равна позиции начала (usr/lib или usr///lib)
            // удаляем слэши чтобы отделить директорию от имени файла
            while (*slashpos == '/')
            {
                *slashpos = '\0';
                slashpos --;
            }
        }
        else
        {
            // слэш в начале (/usr)
            // оставим его одного чтобы получить /
            path[1] = '\0';
        }

        return path;
    }

    return 0;
}