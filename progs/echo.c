#include "../sdk/libc/stdio.h"
#include "../sdk/libc/sys_files.h"
#include "../sdk/sys/syscalls.h"

size_t _strlen (const char *__s)
{
    size_t size = 0;

    while(*__s++)
    {
		size++;
    }

	return size;
}

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

    int fd = open_file(path, FILE_OPEN_FLAG_CREATE | FILE_OPEN_MODE_WRITE_ONLY, 0xFFF);

    if (fd == -1) {
        printf("Can't open file with path %s", path);
    }

    write(fd, content, _strlen(content));

    close(fd);

    return 0;
}