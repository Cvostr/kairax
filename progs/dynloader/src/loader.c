#include <syscalls.h>
#include <sys/stat.h>
#include "loader.h"
#include "string.h"

uint64_t args_info[2];
uint64_t aux_vector[20];
int cfd;

#define FD_CWD		-2
#define DIRFD_IS_FD  1
#define FILE_OPEN_MODE_READ_ONLY    00000000

#define PROTECTION_WRITE_ENABLE    0b1
#define PROTECTION_EXEC_ENABLE     0b10

extern void linker_entry();

void loader() {

    cfd = syscall_open_file(-2, "/dev/console", 00000002, 0);

    int fd = aux_vector[AT_EXECFD];
    int argc = args_info[0];
    char** argv = (char**) args_info[1];

    struct object_data* main_obj = load_object_data_fd(fd, 0);

    // Закрыть файл
    syscall_close(fd);
    
    void (*func)(int, char**) = (void*) aux_vector[AT_ENTRY];
    func(argc, argv);
}

struct object_data* load_object_data_fd(int fd, int shared) {

    struct stat file_stat;
    memset(&file_stat, 0, sizeof(struct stat));
    int rc = syscall_fdstat(fd, 0, &file_stat, DIRFD_IS_FD);

    if (rc != 0) {
        return NULL;
    }

    char* file_buffer = syscall_process_map_memory(NULL, file_stat.st_size, PROTECTION_WRITE_ENABLE, 0);
    memset(file_buffer, 0, file_stat.st_size);

    // Чтение файла
    rc = syscall_read(fd, file_buffer, file_stat.st_size);

    // Загрузка файла
    struct object_data* result = load_object_data(file_buffer, shared);   

    // Освобождение памяти
    //syscall_process_unmap_memory(file_buffer, file_stat.st_size);

    return result;
}

// TODO: should be synchronized
uint64_t shared_objects_brk = 0x7fcc372d7000;

struct object_data* load_object_data(char* data, int shared) {

    struct object_data* obj_data = 
                        (struct object_data*) syscall_process_map_memory(NULL, sizeof(struct object_data), 1, 0);
    memset(obj_data, 0, sizeof(struct object_data));

    if (shared) {
        // Сохранить адрес начала
        obj_data->base = shared_objects_brk;
    }

    struct elf_header* elf_header = (struct elf_header*) data;

    for (uint32_t i = 0; i < elf_header->section_header_entries_num; i ++) {
        // Получить описание секции
        struct elf_section_header_entry* sehentry = elf_get_section_entry(data, i);
        // Получить название секции
        char* section_name = elf_get_string_at(data, sehentry->name_offset);

        if (strcmp(section_name, ".dynamic") == 0) {

            // Сохранить все данные секции
            obj_data->dynamic_section = syscall_process_map_memory(NULL, sehentry->size, PROTECTION_WRITE_ENABLE, 0);
            memcpy(obj_data->dynamic_section, data + sehentry->offset, sehentry->size);
            obj_data->dynamic_sec_size = sehentry->size;

            // Поиск и сохранение адресов важных подсекций
            for (size_t dyn_i = 0; dyn_i < sehentry->size; dyn_i += sizeof(struct elf_dynamic)) {
                struct elf_dynamic* dynamic = (struct elf_dynamic*) ( data + sehentry->offset + dyn_i );

                if (shared == 0) {
                if (dynamic->tag == ELF_DT_PLTGOT) {
                    // Есть секция GOT PLT
                    struct got_plt* got = (struct got_plt*) dynamic->d_un.addr;
                    got->arg = got;
                    got->linker_ip = linker_entry;
                    got->unused = obj_data;
                }
                }
            }
        } else if (strcmp(section_name, ".dynsym") == 0) {
            
            obj_data->dynsym = (struct elf_symbol*)syscall_process_map_memory(NULL, sehentry->size, 1, 0);
            memcpy(obj_data->dynsym, data + sehentry->offset, sehentry->size);

            obj_data->dynsym_size = sehentry->size;

        } else if (strcmp(section_name, ".dynstr") == 0) {
            obj_data->dynstr = (struct elf_symbol*)syscall_process_map_memory(NULL, sehentry->size, 1, 0);
            memcpy(obj_data->dynstr, data + sehentry->offset, sehentry->size);

            obj_data->dynstr_size = sehentry->size;

        } else if (strcmp(section_name, ".rela.plt") == 0) {
            obj_data->rela = (struct elf_rela*)syscall_process_map_memory(NULL, sehentry->size, 1, 0);
            memcpy(obj_data->rela, data + sehentry->offset, sehentry->size);

            obj_data->rela_size = sehentry->size;
        }
    }

    for (size_t i = 0; i < obj_data->dynamic_sec_size; i += sizeof(struct elf_dynamic)) {
        struct elf_dynamic* dynamic = (struct elf_dynamic*) (obj_data->dynamic_section + i);
        
        if (dynamic->tag == ELF_DT_NEEDED) {
            // Название разделяемой библиотеки
            char* object_name = obj_data->dynstr + dynamic->d_un.val;  

            int libfd = open_shared_object_file(object_name);

            if (libfd > 0) {
                syscall_write(cfd, "w\4LIB\n", 6);  
                struct object_data* dependency = load_object_data_fd(libfd, 1);
                strcpy(dependency->name, object_name);

                // Сохранить зависимость
                obj_data->dependencies[obj_data->dependencies_count ++] = dependency;

                syscall_close(libfd);
            }   
        }
    }

    return obj_data;
}

int open_shared_object_file(const char* fname)
{
    char fpname[100];
    strcpy(fpname, "libs/");
    strcat(fpname, fname);

    /*char name_log[15];
            name_log[0] = 'w';
            name_log[1] = strlen(fpname);
            strcpy(name_log + 2, fpname);
            syscall_write(cfd, name_log, 14);*/

    return syscall_open_file(FD_CWD, fpname, FILE_OPEN_MODE_READ_ONLY, 0);
}