#include "stdlib.h"
#include "stddef.h"
#include "string.h"
#include "errno.h"

// https://www.opennet.ru/man.shtml?topic=putenv&category=3&russian=0
// https://www.man7.org/linux/man-pages/man3/putenv.3.html

static char** __orig_env = NULL;
extern char** __environ;

int putenv(char *string) 
{
    char* val = strchr(string, '=');
    size_t namelen = 0;
    int remove = 0;
    int pos = 0;
    char** envp;
    size_t envc = 0;

    // Если в __orig_env находится NULL, значит эта функция ни разу не выполнялась
    // Сохраняем туда адрес env, размеченный при запуске процесса
    if (__orig_env == NULL) {
        __orig_env = __environ;
    }

    int reallocated = (__orig_env != __environ);

    if (val == NULL) 
    {
        // значение не передано - значит надо удалить по имени
        namelen = strlen(string);
        remove = 1;
    } else {
        namelen = val - string;
    }

    if (__environ) 
    {
        for (envp = __environ; (*envp != NULL); envp ++) 
        {

            if (strncmp(*envp, string, namelen) == 0 && (*envp)[namelen] == '=') 
            {
                // TODO: check and fix
                if (remove == 1)
                {
                    while (*(envp + 1) != NULL) {
                        envp[0] = envp[1];
                        envp++;
                    }
                    envp[0] = NULL;
                    return 0;
                }

                // Выполняем замену
                *envp = string;
                return 0;
            }

            envc++;
        }
    }

    // значение не было передано, при этом ключа для замены не было - выходим
    if (val == NULL) {
        return 0;
    }

    char** prev_addr = reallocated ? __environ : 0;
    char** reallocated_addr = realloc(prev_addr, (envc + 2) * sizeof(char*));
    if (reallocated_addr != NULL) {
        
        if (!reallocated) {
            // При первом расширении необходимо перенести данные из буфера, созданного ядром
            memcpy(reallocated_addr, __environ, sizeof(char*) * envc);
        }

        reallocated_addr[envc] = string;
        reallocated_addr[envc + 1] = NULL;

        // Теперь используем новую таблицу env
        __environ = reallocated_addr;

    } else {
        errno = ENOMEM;
        return -1;
    }

    return 0;
}