#include "../../sdk/sys/syscalls.h"
#include "../../sdk/libc/stat.h"

char file_buffer[40000];

void _start(int argc, char** argv) {
    if (argc < 1) {
        
    }

    int fd = (int) argv[0];
    struct stat file_stat;

    // Получение информации
    int rc = syscall_fdstat(fd, "", &file_stat, 1);

    // Чтение файла
    rc = syscall_read(fd, file_buffer, file_stat.st_size);
}