#include "stdlib.h"
#include "stddef.h"
#include "string.h"
#include "errno.h"

extern char** __environ;

extern char *getenv(const char *name)
{
    char** envp;

    if (__environ == NULL || name == NULL)
    {
        return NULL;
    }

    size_t namelen = strlen(name);
    for (envp = __environ; (*envp != NULL); envp ++) 
    {
        if (strncmp(*envp, name, namelen) == 0 && (*envp)[namelen] == '=') 
        {
            return *envp + namelen + 1;
        }
    }

    return NULL;
}

int unsetenv(const char *name)
{
    if (name == NULL) {
        errno = EINVAL;    
        return -1;
    }

    if (name[0] == 0 || strchr(name, '=') != NULL)
    {
        errno = EINVAL;
        return -1;
    }

    return putenv((char*) name);
}

int setenv(const char *name, const char *value, int overwrite)
{
    if (name == NULL) {
        errno = EINVAL;
        return -1;    
    }

    if (name[0] == 0 || strchr(name, '=') != NULL)
    {
        errno = EINVAL;
        return -1;
    }

    char* val = getenv(name);
    if (val != NULL) 
    {
        if (!overwrite) {
            return 0;
        }

        unsetenv(name);
    }

    size_t envstr_len = strlen(name) + strlen(value) + 2;
    char* envstr = malloc(envstr_len);
    memset(envstr, 0, envstr_len);
    strcpy(envstr, name);
    strcat(envstr, "=");
    strcat(envstr, value);

    return putenv(envstr);
}