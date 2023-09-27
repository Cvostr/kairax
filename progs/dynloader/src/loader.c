#include <syscalls.h>
#include <sys/stat.h>
#include "include/elf.h"
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
    char* file_buffer = NULL;
    file_buffer = syscall_process_map_memory(NULL, file_stat.st_size, 1, 0);
    memset(file_buffer, 0, file_stat.st_size);

    // Чтение файла
    rc = syscall_read(fd, file_buffer, file_stat.st_size);

    // Закрыть файл
    syscall_close(fd);

    struct elf_header* elf_header = (struct elf_header*) file_buffer;
    struct object_data* main_object_data = 
                        (struct object_data*) syscall_process_map_memory(NULL, sizeof(struct object_data), 1, 0);

    for (uint32_t i = 0; i < elf_header->section_header_entries_num; i ++) {
        // Получить описание секции
        struct elf_section_header_entry* sehentry = elf_get_section_entry(file_buffer, i);
        // Получить название секции
        char* section_name = elf_get_string_at(file_buffer, sehentry->name_offset);

        if (strcmp(section_name, ".dynamic") == 0) {

            main_object_data->dynamic_section = syscall_process_map_memory(NULL, sehentry->size, 1, 0);
            memcpy(main_object_data->dynamic_section, file_buffer + sehentry->offset, sehentry->size);

            // Поиск и сохранение адресов важных подсекций
            for (size_t i = 0; i < sehentry->size; i += sizeof(struct elf_dynamic)) {
                struct elf_dynamic* dynamic = (struct elf_dynamic*) (i + sehentry->offset + file_buffer);
                /*if (dynamic->tag == ELF_DT_NEEDED) {
                    char* object_name = elf_get_string_at(file_buffer, dynamic->d_un.val);
                    syscall_write(cfd, temp, 12);
                }*/
                if (dynamic->tag == ELF_DT_PLTGOT) {
                    // Есть секция GOT PLT
                    struct got_plt* got = (struct got_plt*) dynamic->d_un.addr;
                    got->arg = got;
                    got->linker_ip = linker_entry;
                    got->unused = main_object_data;
                }
            }
        } else if (strcmp(section_name, ".dynsym") == 0) {
            
            main_object_data->dynsym = (struct elf_symbol*)syscall_process_map_memory(NULL, sehentry->size, 1, 0);
            memcpy(main_object_data->dynsym, file_buffer + sehentry->offset, sehentry->size);

            main_object_data->dynsym_size = sehentry->size;

        } else if (strcmp(section_name, ".dynstr") == 0) {
            main_object_data->dynstr = (struct elf_symbol*)syscall_process_map_memory(NULL, sehentry->size, 1, 0);
            memcpy(main_object_data->dynstr, file_buffer + sehentry->offset, sehentry->size);

            main_object_data->dynstr_size = sehentry->size;

        } else if (strcmp(section_name, ".rela.plt") == 0) {
            main_object_data->rela = (struct elf_rela*)syscall_process_map_memory(NULL, sehentry->size, 1, 0);
            memcpy(main_object_data->rela, file_buffer + sehentry->offset, sehentry->size);

            main_object_data->rela_size = sehentry->size;
        }
    }

    syscall_process_unmap_memory(file_buffer, file_stat.st_size);
    
    void (*func)(int, char**) = (void*) aux_vector[AT_ENTRY];
    func(argc, argv);
}

struct object_data* load_object_data(char* data);

struct object_data* load_object_data_fd(int fd) {

    struct stat file_stat;
    int rc = syscall_fdstat(fd, 0, &file_stat, 1);

    char* file_buffer = syscall_process_map_memory(NULL, file_stat.st_size, 1, 0);
    memset(file_buffer, 0, file_stat.st_size);

    // Чтение файла
    rc = syscall_read(fd, file_buffer, file_stat.st_size);

    return load_object_data(file_buffer);
}

struct object_data* load_object_data(char* data) {
    struct object_data* obj_data = 
                        (struct object_data*) syscall_process_map_memory(NULL, sizeof(struct object_data), 1, 0);

    struct elf_header* elf_header = (struct elf_header*) data;

    return obj_data;
}