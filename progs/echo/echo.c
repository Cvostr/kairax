#include "stdio.h"
#include "unistd.h"
#include "fcntl.h"
#include "string.h"

int main(int argc, char** argv) {

    char* path = "/dev/console";
    char* content = NULL;

    int path_mode = 0;

    for (int i = 1; i < argc; i ++) {

        if (i == 1) {
            content = argv[i];
        } else {

            if (argv[i][0] == '.') {
                path_mode = 1;
            } else if (path_mode == 1) {
                path = argv[i];
                path_mode = 0;
            }

        }
    }

    if (content == NULL) {
        return 1;
    }

    int fd = creat(path, 0777);

    if (fd == -1) {
        printf("Can't open file with path %s", path);
    }

    write(fd, content, strlen(content));

    close(fd);

    return 0;
}