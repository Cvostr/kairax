#include "stdio.h"
#include "unistd.h"
#include "string.h"
#include "fcntl.h"
#include "stdbool.h"
#include "errno.h"
#include "sys/stat.h"
#include "time.h"

void stringify_perm(int perm, char* str)
{   
    // Read
    str[0] = ((perm & 4) == 4) ? 'r' : '-';
    // Write
    str[1] = ((perm & 2) == 2) ? 'w' : '-';
    // Exec
    str[2] = ((perm & 1) == 1) ? 'x' : '-';
}

char* str_filetype(mode_t mode)
{
    if (S_ISREG(mode))
    {
        return "regular file";
    }
    if (S_ISDIR(mode))
    {
        return "directory";
    }
    if (S_ISSOCK(mode))
    {
        return "socket";
    }
    if (S_ISLNK(mode))
    {
        return "symbolic link";
    }
    if (S_ISCHR(mode))
    {
        return "character special file";
    }

    return "unknown";
}

void print_date(char* field, struct tm* t)
{
    printf("%s: %02i:%02i:%02i %02i-%02i-%i\n", 
            field,
            t->tm_hour, 
			t->tm_min,
			t->tm_sec,
			t->tm_mday,
			t->tm_mon,
			t->tm_year + 1900
    );
}

int main(int argc, char** argv)
{
    int dereference = 0;
    char* file = NULL;

    int fd;
    int rc;

    if (argc < 2)
    {
        printf("stat: missing operand\n");
        return 1;
    }

    for (int i = 1; i < argc; i ++)
    {
        char* arg = argv[i];

        if (strcmp(arg, "-L") == 0)
        {
            dereference = 1;
        }
        else 
        {
            file = arg;
        }
    }

    int openflags = O_RDONLY;
    if (dereference == 0)
    {
        openflags |= (O_NOFOLLOW | O_PATH);
    }

    fd = open(file, openflags, 0);
    if (fd == -1) 
    {
        printf("stat: Can't open file %s: %s\n", file, strerror(errno));
        return 1;
    }

    struct stat sta;
    memset(&sta, 0, sizeof(sta));
    rc = fstat(fd, &sta);
    if (rc == -1)
    {
        printf("stat: Can't stat %s: %s\n", file, strerror(errno));
        return 1;
    }

    printf("File: %s\n", file);
    printf("Size: %lu Blocks: %lu IO Block: %lu %s\n", sta.st_size, sta.st_blocks, sta.st_blksize, str_filetype(sta.st_mode));
    printf("Device: %i Inode: %i Links: %i\n", sta.st_rdev, sta.st_ino, sta.st_nlink);

    int perm = sta.st_mode & 0777;
    char perm_str[9 + 1];
    memset(perm_str, 0, sizeof(perm_str));
    stringify_perm(perm >> 6, perm_str);
    stringify_perm((perm >> 3) & 07, perm_str + 3);
    stringify_perm(perm & 07, perm_str + 6);

    printf("Access: (%s) Uid: %i Gid: %i\n", perm_str, sta.st_uid, sta.st_gid);

    struct tm* t = gmtime(&sta.st_atim.tv_sec);
    print_date("Access", t);

    t = gmtime(&sta.st_mtim.tv_sec);
    print_date("Modify", t);

    t = gmtime(&sta.st_ctim.tv_sec);
    print_date("Change", t);


    return 0;
}