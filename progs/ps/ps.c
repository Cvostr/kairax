#include "stdio.h"
#include "dirent.h"
#include "stdlib.h"
#include "string.h"
#include "stdbool.h"
#include "sys/types.h"
#include "fcntl.h"
#include "unistd.h"
#include "errno.h"
#include "sys/stat.h"

#define STR_BUFFERSZ        100

struct process_desc {
    pid_t pid;
    uid_t uid;
    int status;
    char *name;
};

int to_pid(const char *str, pid_t *pid) 
{
    pid_t res;
    char *endptr;

    if (str == NULL || *str == '\0') 
        return FALSE;
    
    res = strtol(str, &endptr, 10);
    
    if (*endptr == '\0') 
    {
        *pid = res;
        return TRUE;
    }

    return FALSE;
}

int desc_append(struct process_desc ***descs, size_t *ndescs, struct process_desc *desc)
{
    size_t size_old = *ndescs;
    (*ndescs) ++;

    struct process_desc **_descs = malloc(*ndescs * sizeof(struct process_desc *));
    if (*descs != NULL)
    {
        memcpy(_descs, *descs, (size_old) * sizeof(struct process_desc *));
        free(*descs);
    }

    _descs[size_old] = desc;

    *descs = _descs;
    return 0;
}

// Считать из виртуального файла с 0 терминированием
ssize_t read_str_from_virtual_file(FILE *f, char **out) 
{
    ssize_t total = 0;
    ssize_t n = 0;
    char buffer[STR_BUFFERSZ];
    char *result = NULL;
    char *new;

    while ((n = fread(buffer, 1, STR_BUFFERSZ, f)) > 0) 
    {
        new = malloc(total + n + 1);
        new[total + n] = '\0';

        if (result != NULL)
        {
            memcpy(new, result, total);
            free(result);
        }

        result = new;
        memcpy(result + total, buffer, n);
        total += n;
    }

    *out = result;
    return total;
}

int process_make_desc(ino_t ino, pid_t pid, struct process_desc *desc) 
{
    int statusfd;
    int cmdlinefd;

    desc->pid = pid;

    cmdlinefd = openat(ino, "cmdline", O_RDONLY, 0);
    if (cmdlinefd == -1)
    {
        return -1;
    }

    statusfd = openat(ino, "status", O_RDONLY, 0);
    if (cmdlinefd == -1)
    {
        close(cmdlinefd);
        return -1;
    }

    // приводим cmdline к libc типу и читаем строку
    FILE *fcmdline = fdopen(cmdlinefd, "r");
    read_str_from_virtual_file(fcmdline, &desc->name);
    fclose(fcmdline);

    // приводим statusfd к libc типу и читаем строку
    FILE *fstatus = fdopen(statusfd, "r");
    //read_str_from_virtual_file(statusfd, &desc->name);
    fclose(fstatus);

    return 0;
}

int main(int argc, char** argv) 
{
    size_t ndescs = 0;
    struct process_desc **descs = NULL;

    // пробуем открыть директорию /proc
    DIR* dir = opendir("/proc");
    if (dir == NULL)
    {
        printf("Unable to open /proc directory");
        return 1;
    }

    int procfd = dirfd(dir);

    pid_t pid;
    struct dirent* dr;
    while ((dr = readdir(dir)) != NULL) 
    {
        if ((strcmp(dr->d_name, ".") == 0 || strcmp(dr->d_name, "..") == 0))
        {
            continue;
        }

        if (dr->d_type == DT_DIR && to_pid(dr->d_name, &pid))
        {
            //printf("processing %s ino %lu\n", dr->d_name, dr->d_ino);
            int processfd = openat(procfd, dr->d_name, O_RDONLY | O_DIRECTORY, 0);
            if (processfd == -1)
            {
                printf("Unable to open process dir %s: %s\n", dr->d_name, strerror(errno));
            }

            struct process_desc *desc = malloc(sizeof(struct process_desc));

            if (process_make_desc(processfd, pid, desc) == 0)
            {
                // добавить при успехе
                desc_append(&descs, &ndescs, desc);
            }
            else
            {
                printf("Error obtaining information of process %lu\n", pid);
                free(desc);
            }

            close(processfd);
        }
    }

    //printf("total processes %i\n", ndescs);
    printf("    PID  CMD\n");
    for (size_t i = 0; i < ndescs; i ++)
    {
        struct process_desc *desc = descs[i];
        printf("%7i  %s\n", desc->pid, desc->name);
    }

    return 0;
}