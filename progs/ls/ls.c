#include "stdio.h"
#include "errno.h"
#include "unistd.h"
#include "fcntl.h"
#include "dirent.h"
#include "sys/stat.h"
#include "time.h"

int main(int argc, char** argv) {

    int all = 0;
    int rc;

    char* path = "";
    for (int i = 1; i < argc; i ++) {
        if (argv[i][0] != '-') {
            path = argv[i];
        } else {
            if (argv[i][1] == 'a') {
                all = 1;
            }
        }
    }

    int dirfd = open(path, O_RDONLY, 0);
    DIR* dir = opendir(path);

    if (dirfd == -1) {
        printf("Can't open %s", path);
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

            printf("%-4s %03o %9i %02i:%02i:%02i %i:%i:%i %s\n", 
                (dr->d_type == DT_REG) ? "FILE" : "DIR",
                perm,
                file_stat.st_size,
                t->tm_hour, 
				t->tm_min,
				t->tm_sec,
				t->tm_mday,
				t->tm_mon,
				t->tm_year + 1900,
                dr->d_name);
        
        } else {
        
            printf("%4s %s\n", (dr->d_type == DT_REG) ? "FILE" : "DIR", dr->d_name);
        
        }
    }    

    closedir(dir);

    return 0;
}