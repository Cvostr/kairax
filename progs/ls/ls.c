#include "stdio.h"
#include "errno.h"
#include "unistd.h"
#include "fcntl.h"
#include "dirent.h"
#include "sys/stat.h"

int main(int argc, char** argv) {

    int all = 0;

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

    if (dirfd == -1) {
        printf("Can't open %s", path);
        return 1;
    }

    struct stat dirstat;
    int rc = fstat(dirfd, &dirstat);

    struct dirent dr;
    while (readdir(dirfd, &dr) == 1) {

        if (all) {
            rc = fstatat(dirfd, dr.d_name, &dirstat, 0);
            int perm = dirstat.st_mode & 0777;
            if (rc == -1) {
                printf("Error! Can't stat file %s, error=%i\n", dr.d_name, errno);
            }
            printf("%s %o %s %i\n", (dr.d_type == DT_REG) ? "FILE" : "DIR ", perm, dr.d_name, dirstat.st_size);
        } else {
            printf("%s %s\n", (dr.d_type == DT_REG) ? "FILE" : "DIR ", dr.d_name);
        }
    }    

    return 0;
}