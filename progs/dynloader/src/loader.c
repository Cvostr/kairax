#include <syscalls.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "loader.h"
#include "string.h"
#include "stdio.h"
#include "object_data.h"
#include "align.h"
#include "linker.h"

uint64_t            args_info[2];
uint64_t            aux_vector[20];
struct object_data* root;

#define MAP_ANONYMOUS	0x20

#define DIRFD_IS_FD  0x1000
#define FILE_OPEN_MODE_READ_ONLY    00000000

#define PAGE_PROTECTION_READ_ENABLE     0x01
#define PAGE_PROTECTION_WRITE_ENABLE    0x02
#define PAGE_PROTECTION_EXEC_ENABLE     0x04
#define PAGE_PROTECTION_USER            0x08

#define SO_BASE                     (void*) 0x7fcc372d7000ULL
#define PAGE_SIZE                   aux_vector[AT_PAGESZ]

extern void linker_entry();

void loader() {

    int fd = aux_vector[AT_EXECFD];
    int argc = args_info[0];
    char** argv = (char**) args_info[1];

    // Загрузить главный объект
    root = load_object_data_fd(fd, 0);

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
        printf("Error! can't stat object fd!\n");
        return NULL;
    }

    // Выделить память под файл
    char* file_buffer = syscall_map_memory(NULL, file_stat.st_size, PAGE_PROTECTION_WRITE_ENABLE, MAP_ANONYMOUS, -1, 0);
    memset(file_buffer, 0, file_stat.st_size);

    // Чтение файла
    rc = syscall_read(fd, file_buffer, file_stat.st_size);

    // Загрузка файла
    struct object_data* result = load_object_data(file_buffer, shared);   

    // Освобождение памяти
    syscall_process_unmap_memory(file_buffer, file_stat.st_size);

    return result;
}

struct object_data* load_object_data(char* data, int shared) {

    struct object_data* obj_data = (struct object_data*) syscall_map_memory(
                                            NULL,
                                            sizeof(struct object_data),
                                            PAGE_PROTECTION_WRITE_ENABLE,
                                            MAP_ANONYMOUS, -1, 0);

    memset(obj_data, 0, sizeof(struct object_data));

    struct elf_header* elf_header = (struct elf_header*) data;

    if (shared) {

        // Подсчитать размер кода
        for (uint32_t i = 0; i < elf_header->prog_header_entries_num; i ++) {
            struct elf_program_header_entry* pehentry = elf_get_program_entry(data, i);
                
            // Получить размер и максимальный адрес
            if (pehentry->type == ELF_SEGMENT_TYPE_LOAD) {

                uint64_t vaddr_aligned = align_down(pehentry->v_addr, PAGE_SIZE); // выравнивание в меньшую сторону
                uint64_t end_aligned = align(pehentry->v_addr + pehentry->p_memsz, PAGE_SIZE);
                uint64_t aligned_size = end_aligned - vaddr_aligned;

                if (vaddr_aligned + aligned_size > obj_data->size) {
                    obj_data->size = pehentry->v_addr + aligned_size;
                }
            }
        }

        // Выделить память под код
        obj_data->base = (uint64_t)syscall_map_memory(
            SO_BASE,
            obj_data->size,
            PAGE_PROTECTION_WRITE_ENABLE | PAGE_PROTECTION_EXEC_ENABLE,
            MAP_ANONYMOUS, -1, 0);

        // Расположить код в памяти
        for (uint32_t i = 0; i < elf_header->prog_header_entries_num; i ++) {
            struct elf_program_header_entry* pehentry = elf_get_program_entry(data, i);
            if (pehentry->type == ELF_SEGMENT_TYPE_LOAD) {
                // Адрес, по которому будут загружены данные
                uint64_t vaddr = pehentry->v_addr + obj_data->base;
                uint64_t vaddr_aligned = align_down(vaddr, PAGE_SIZE); // выравнивание в меньшую сторону
                uint64_t end_aligned = align(vaddr + pehentry->p_memsz, PAGE_SIZE);
                uint64_t aligned_size = end_aligned - vaddr_aligned;

                int protection = 0;
                if (pehentry->flags & ELF_SEGMENT_FLAG_EXEC) {
                    protection |= PAGE_PROTECTION_EXEC_ENABLE;
                }

                if (pehentry->flags & ELF_SEGMENT_FLAG_WRITE) {
                    protection |= PAGE_PROTECTION_WRITE_ENABLE;
                }
                
                // Заполнить выделенную память нулями
                memset((void*) vaddr, 0, pehentry->p_memsz);
                // Копировать фрагмент программы в память
                memcpy((void*) vaddr, data + pehentry->p_offset, pehentry->p_filesz); 
                // Защитить память
                syscall_protect_memory((void*) vaddr, pehentry->p_memsz, protection); 
            }
        }
    }

    for (uint32_t i = 0; i < elf_header->section_header_entries_num; i ++) {
        // Получить описание секции
        struct elf_section_header_entry* sehentry = elf_get_section_entry(data, i);
        // Получить название секции
        char* section_name = elf_get_string_at(data, sehentry->name_offset);

        if (strcmp(section_name, ".dynamic") == 0) {

            // Сохранить все данные секции
            obj_data->dynamic_section = syscall_map_memory(NULL, sehentry->size, PAGE_PROTECTION_WRITE_ENABLE, MAP_ANONYMOUS, -1, 0);
            memcpy(obj_data->dynamic_section, data + sehentry->offset, sehentry->size);
            obj_data->dynamic_sec_size = sehentry->size;

            // Поиск и сохранение адресов важных подсекций
            for (size_t dyn_i = 0; dyn_i < sehentry->size; dyn_i += sizeof(struct elf_dynamic)) {
                struct elf_dynamic* dynamic = (struct elf_dynamic*) ( data + sehentry->offset + dyn_i );

                switch (dynamic->tag) {
                    case ELF_DT_PLTGOT:
                        // Есть секция GOT PLT
                        struct got_plt* got = (struct got_plt*) (obj_data->base + dynamic->d_un.addr);
                        got->arg = got;
                        got->linker_ip = linker_entry;
                        got->unused = obj_data;
                        break;
                }
            }
        } else if (strcmp(section_name, ".dynsym") == 0) {
            
            obj_data->dynsym = (struct elf_symbol*)syscall_map_memory(
                NULL,
                sehentry->size,
                PAGE_PROTECTION_WRITE_ENABLE,
                MAP_ANONYMOUS, -1, 0);

            memcpy(obj_data->dynsym, data + sehentry->offset, sehentry->size);

            obj_data->dynsym_size = sehentry->size;

        } else if (strcmp(section_name, ".dynstr") == 0) {
            obj_data->dynstr = (struct elf_symbol*)syscall_map_memory(
                NULL,
                sehentry->size,
                PAGE_PROTECTION_WRITE_ENABLE,
                MAP_ANONYMOUS, -1, 0);

            memcpy(obj_data->dynstr, data + sehentry->offset, sehentry->size);

            obj_data->dynstr_size = sehentry->size;

        } else if (strcmp(section_name, ".rela.plt") == 0) {
            obj_data->plt_rela = (struct elf_rela*)syscall_map_memory(
                NULL,
                sehentry->size,
                PAGE_PROTECTION_WRITE_ENABLE,
                MAP_ANONYMOUS, -1, 0);
                
            memcpy(obj_data->plt_rela, data + sehentry->offset, sehentry->size);

            obj_data->plt_rela_size = sehentry->size;
        } else if (strcmp(section_name, ".rela.dyn") == 0) {
            obj_data->rela = (struct elf_rela*) syscall_map_memory(
                NULL,
                sehentry->size,
                PAGE_PROTECTION_WRITE_ENABLE,
                MAP_ANONYMOUS, -1, 0);

            memcpy(obj_data->rela, data + sehentry->offset, sehentry->size);
            obj_data->rela_size = sehentry->size;
        }
    }

    // Первоначальная обработка перемещений PLT
    for (size_t i = 0; i < obj_data->plt_rela_size / sizeof(struct elf_rela); i ++) {
        // Указатель на перемещение
        struct elf_rela* rela = obj_data->plt_rela + i;

        // Тип перемещения - прыжок на PLT.
        // Если у нас разделяемая библиотека - код GOT с адресами по умолчанию будет перенесен 
        uint64_t* value = (uint64_t*) (obj_data->base + rela->offset);

        // Сдвинуть relocation
        if (*value > 0)
            *value += obj_data->base;
        
        rela->offset += obj_data->base;
    }

    // Загрузка зависимостей объекта
    for (size_t i = 0; i < obj_data->dynamic_sec_size; i += sizeof(struct elf_dynamic)) {
        struct elf_dynamic* dynamic = (struct elf_dynamic*) (obj_data->dynamic_section + i);
        
        if (dynamic->tag == ELF_DT_NEEDED) {
            // Название разделяемой библиотеки
            char* object_name = obj_data->dynstr + dynamic->d_un.val;  

            // Найти и открыть файл
            int libfd = open_shared_object_file(object_name);

            if (libfd < 0) {
                printf("Error locating shared library %s\n", object_name);
            }

            if (libfd > 0) {
                // Загрузить библиотеку
                struct object_data* dependency = load_object_data_fd(libfd, 1);
                strcpy(dependency->name, object_name);

                // Сохранить зависимость
                obj_data->dependencies[obj_data->dependencies_count ++] = dependency;

                // Закрыть файл библиотеки
                syscall_close(libfd);
            }   
        }
    }

    // Обработка перемещений
    for (size_t i = 0; i < obj_data->rela_size / sizeof(struct elf_rela); i ++) {
        // Указатель на перемещение
        struct elf_rela* rela = obj_data->rela + i;
        int relocation_type = ELF64_R_TYPE(rela->info);
        int relocation_sym_index = ELF64_R_SYM(rela->info);
        uint64_t* value = (uint64_t*) (obj_data->base + rela->offset);
        //printf("type %i, off %i, num %i\n", relocation_type, rela->offset, relocation_sym_index);

        struct elf_symbol* sym = (struct elf_symbol*) obj_data->dynsym + relocation_sym_index;
        char* name = NULL;

        switch (relocation_type) {
            case R_X86_64_RELATIVE:
                // Прибавить смещение
                *value += obj_data->base;
                break;
            case R_X86_64_COPY:
                // Скопировать содержимое символа
                name = obj_data->dynstr + sym->name;
                // Ищем символ в зависимостях
                struct object_data* dep = NULL;
                struct elf_symbol* dep_symbol = look_for_symbol(obj_data, name, &dep, MODE_LOOK_IN_DEPS);

                if (dep_symbol == NULL) {
                    printf("Can't find symbol %s\n", name);
                    // todo : error
                }

                // Копировать
                memcpy((void*)obj_data->base + sym->value, (void*)dep->base + dep_symbol->value, dep_symbol->size);

                break;
            case R_X86_64_GLOB_DAT:
                // Записать адрес на символ в GOT
                name = obj_data->dynstr + sym->name;

                *value = obj_data->base + sym->value;
                break;
        }
    }

    return obj_data;
}

int open_shared_object_file(const char* fname)
{
    // Открыть в рабочей папке
    int fd = syscall_open_file(AT_FDCWD, fname, FILE_OPEN_MODE_READ_ONLY, 0);
    
    if (fd < 0) {
        // Открыть в системной папке
        char fpname[300];
        strcpy(fpname, "/libs/");
        strcat(fpname, fname);
        fd = syscall_open_file(AT_FDCWD, fpname, FILE_OPEN_MODE_READ_ONLY, 0);
    }

    return fd;
}