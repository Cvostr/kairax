#include "../../sdk/sys/syscalls.h"
#include <sys/stat.h>
#include "elf.h"
#include "string.h"

uint64_t args_info[2];
uint64_t aux_vector[20];
int cfd;

extern void linker_entry();

void loader() {

    cfd = syscall_open_file(-2, "/dev/console", 00000002, 0);

    int fd = aux_vector[AT_EXECFD];
    struct stat file_stat;
    int argc = args_info[0];
    char** argv = (char**) args_info[1];

    // Получение информации
    int rc = syscall_fdstat(fd, 0, &file_stat, 1);

    // Выделить память
    char* file_buffer = 0x20000000;
    syscall_process_map_memory(file_buffer, file_stat.st_size, 1, 0);
    memset(file_buffer, 0, file_stat.st_size);

    // Чтение файла
    rc = syscall_read(fd, file_buffer, file_stat.st_size);

    struct elf_header* elf_header = (struct elf_header*) file_buffer;

    for (uint32_t i = 0; i < elf_header->section_header_entries_num; i ++) {
        // Получить описание секции
        struct elf_section_header_entry* sehentry = elf_get_section_entry(file_buffer, i);
        // Получить название секции
        char* section_name = elf_get_string_at(file_buffer, sehentry->name_offset);

        if (strcmp(section_name, ".got.plt") == 0) {
            // Есть секция GOT PLT
            struct got_plt* got = (struct got_plt*) sehentry->addr;
            got->arg = got;
            got->linker_ip = linker_entry;
        } else if (strcmp(section_name, ".dynamic") == 0) {
            
        }
    }

    void (*func)(int, char**) = (void*) aux_vector[AT_ENTRY];
    func(argc, argv);
}