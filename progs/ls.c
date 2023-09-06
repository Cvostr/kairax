#include "../sdk/libc/stdio.h"
#include "../sdk/libc/sys_files.h"
#include "../sdk/sys/syscalls.h"
#include "../sdk/libc/errno.h"

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

    int dirfd = open_file(path, FILE_OPEN_MODE_READ_ONLY, 0);

    if (dirfd == -1) {
        printf("Can't open %s", path);
        return 1;
    }

    struct stat dirstat;
    int rc = fstat(dirfd, &dirstat);

    struct dirent dr;
    while(readdir(dirfd, &dr) == 1) {

        if (all) {
            rc = stat_at(dirfd, dr.name, &dirstat);
            if (rc == -1) {
                printf("Error! Can't stat file %s, error=%i\n", dr.name, errno);
            }
            printf("TYPE %s, NAME %s, SIZE %i\n", (dr.type == DT_REG) ? "FILE" : "DIR", dr.name, dirstat.st_size);
        } else {
            printf("TYPE %s, NAME %s\n", (dr.type == DT_REG) ? "FILE" : "DIR", dr.name);
        }
    }    

    return 0;
}