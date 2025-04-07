#include "stdio.h"
#include "errno.h"
#include "unistd.h"
#include "fcntl.h"
#include "dirent.h"
#include "sys/stat.h"
#include "string.h"
#include "time.h"

char* to_filetype(int dentry_type)
{
    switch (dentry_type){
        case DT_REG:
            return "f";
        case DT_DIR:
            return "d";
        case DT_FIFO:
            return "p";
        case DT_SOCK:
            return "s";
        case DT_LNK:
            return "l";
    }

    return "";
}

void stringify_perm(int perm, char* str)
{   
    // Read
    str[0] = ((perm & 4) == 4) ? 'r' : '-';
    // Write
    str[1] = ((perm & 2) == 2) ? 'w' : '-';
    // Exec
    str[2] = ((perm & 1) == 1) ? 'x' : '-';
}

char linkbuff[300] = {0,};

int main(int argc, char** argv) {

    int all = 0;
    int rc;

    char* path = "";
    for (int i = 1; i < argc; i ++) {
        if (argv[i][0] != '-') {
            path = argv[i];
        } else {
            if (argv[i][1] == 'l') {
                all = 1;
            }
        }
    }

    int dirfd = open(path, O_RDONLY, 0);
    DIR* dir = opendir(path);

    if (dirfd == -1) {
        printf("Can't open %s, errno=%i\n", path, errno);
        return 1;
    }

    struct stat file_stat;
    struct dirent* dr;

    while ((dr = readdir(dir)) != NULL) {

        if (all) {
        
            rc = fstatat(dirfd, dr->d_name, &file_stat, 0);
            int perm = file_stat.st_mode & 0777;
        
            if (rc == -1) {
                printf("Error! Can't stat file %s, error=%i\n", dr->d_name, errno);
            }
        
            struct tm* t = gmtime(&file_stat.st_ctim.tv_sec);

            char perm_str[9 + 1];
            memset(perm_str, 0, sizeof(perm_str));
            stringify_perm(perm >> 6, perm_str);
            stringify_perm((perm >> 3) & 07, perm_str + 3);
            stringify_perm(perm & 07, perm_str + 6);

            if (dr->d_type == DT_LNK)
            {
                ssize_t linklen = readlinkat(dirfd, dr->d_name, linkbuff, sizeof(linkbuff));
                if (linklen >= 0) {
                    linkbuff[linklen] = 0;
                } else {
                    linkbuff[0] = 0;
                }
            } 
            else
            {
                linkbuff[0] = 0;
            }

            printf("%s %s %i %i %9i %02i:%02i:%02i %02i:%02i:%i %s %s\n", 
                to_filetype(dr->d_type),
                perm_str,
                file_stat.st_uid,
                file_stat.st_gid,
                file_stat.st_size,
                t->tm_hour, 
				t->tm_min,
				t->tm_sec,
				t->tm_mday,
				t->tm_mon,
				t->tm_year + 1900,
                dr->d_name,
                linkbuff
            );
        
        } else {
        
            printf("%s %s\n", to_filetype(dr->d_type), dr->d_name);
        
        }
    }    

    closedir(dir);

    return 0;
}