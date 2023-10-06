#include "stdio.h"
#include "sys/stat.h"
#include "errno.h"
#include "unistd.h"

void fill_mode(int* mode, int value, int read, int write, int exec) {
    if (value & 0b001) *mode |= exec;
    if (value & 0b010) *mode |= write;
    if (value & 0b100) *mode |= read;
}

int main(int argc, char** argv) {

    if (argc < 2) {
        printf("Unspecified file to read\n");
        return 1;
    }

    char* src = argv[2];
    char* mode = argv[1];

    int user_mode = mode[0] - '0';
    int group_mode = mode[1] - '0';
    int others_mode = mode[2] - '0';

    int result_mode = 0;

    fill_mode(&result_mode, user_mode, S_IRUSR, S_IWUSR, S_IXUSR);
    fill_mode(&result_mode, group_mode, S_IRGRP, S_IWGRP, S_IXGRP);
    fill_mode(&result_mode, others_mode, S_IROTH, S_IWOTH, S_IXOTH);

    int rc = chmod(src, result_mode);

    if (rc == -1) {
        printf("chmod failed with %i code\n", errno);
    }

    return 0;
}