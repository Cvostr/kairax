#include <syscalls.h>
#include <sys/stat.h>
#include "loader.h"
#include "string.h"
#include "stdio.h"

uint64_t            args_info[2];
uint64_t            aux_vector[20];
struct object_data* root;
int cfd;

#define FD_CWD		-2
#define DIRFD_IS_FD  1
#define FILE_OPEN_MODE_READ_ONLY    00000000

#define PROTECTION_WRITE_ENABLE    0b1
#define PROTECTION_EXEC_ENABLE     0b10

#define SO_BASE                     (void*) 0x7fcc372d7000ULL
#define PAGE_SIZE                   aux_vector[AT_PAGESZ]

extern void linker_entry();

uint64_t align(uint64_t val, size_t alignment)
{
	if (val < alignment)
		return alignment;

	if (val % alignment != 0) {
        val += (alignment - (val % alignment));
    }

	return val;
}

uint64_t align_down(uint64_t val, size_t alignment)
{
	return val - (val % alignment);
}

void loader() {

    cfd = syscall_open_file(-2, "/dev/console", 00000002, 0);

    int fd = aux_vector[AT_EXECFD];
    int argc = args_info[0];
    char** argv = (char**) args_info[1];

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
        return NULL;
    }

    // Выделить память под файл
    char* file_buffer = syscall_process_map_memory(NULL, file_stat.st_size, PROTECTION_WRITE_ENABLE, 0);
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

    struct object_data* obj_data = 
                        (struct object_data*) syscall_process_map_memory(NULL, sizeof(struct object_data), 1, 0);
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
        obj_data->base = (uint64_t)syscall_process_map_memory(SO_BASE, obj_data->size, PROTECTION_WRITE_ENABLE | PROTECTION_EXEC_ENABLE, 0);

        // Расположить код в памяти
        for (uint32_t i = 0; i < elf_header->prog_header_entries_num; i ++) {
            struct elf_program_header_entry* pehentry = elf_get_program_entry(data, i);
            if (pehentry->type == ELF_SEGMENT_TYPE_LOAD) {
                // Адрес, по которому будут загружены данные
                uint64_t vaddr = pehentry->v_addr + obj_data->base;
                uint64_t vaddr_aligned = align_down(vaddr, PAGE_SIZE); // выравнивание в меньшую сторону
                uint64_t end_aligned = align(vaddr + pehentry->p_memsz, PAGE_SIZE);
                uint64_t aligned_size = end_aligned - vaddr_aligned;

                // Выделить память по адресу
                //syscall_process_map_memory(vaddr, aligned_size, PROTECTION_WRITE_ENABLE | PROTECTION_EXEC_ENABLE, 0);
                // Заполнить выделенную память нулями
                memset((void*) vaddr, 0, pehentry->p_memsz);
                // Копировать фрагмент программы в память
                memcpy((void*) vaddr, data + pehentry->p_offset, pehentry->p_filesz);  
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
            obj_data->dynamic_section = syscall_process_map_memory(NULL, sehentry->size, PROTECTION_WRITE_ENABLE, 0);
            memcpy(obj_data->dynamic_section, data + sehentry->offset, sehentry->size);
            obj_data->dynamic_sec_size = sehentry->size;

            // Поиск и сохранение адресов важных подсекций
            for (size_t dyn_i = 0; dyn_i < sehentry->size; dyn_i += sizeof(struct elf_dynamic)) {
                struct elf_dynamic* dynamic = (struct elf_dynamic*) ( data + sehentry->offset + dyn_i );

                if (dynamic->tag == ELF_DT_PLTGOT) {
                    // Есть секция GOT PLT
                    struct got_plt* got = (struct got_plt*) (obj_data->base + dynamic->d_un.addr);
                    got->arg = got;
                    got->linker_ip = linker_entry;
                    got->unused = obj_data;
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

    for (size_t i = 0; i < obj_data->rela_size / sizeof(struct elf_rela); i ++) {
        struct elf_rela* rela = obj_data->rela + i;

        uint64_t* value = (uint64_t*) (obj_data->base + rela->offset);

        if (*value > 0)
            *value += obj_data->base;
        
        rela->offset += obj_data->base;
    }

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

    return obj_data;
}

int open_shared_object_file(const char* fname)
{
    // Открыть в рабочей папке
    int fd = syscall_open_file(FD_CWD, fname, FILE_OPEN_MODE_READ_ONLY, 0);
    
    if (fd < 0) {
        // Открыть в системной папке
        char fpname[100];
        strcpy(fpname, "/libs/");
        strcat(fpname, fname);
        fd = syscall_open_file(FD_CWD, fpname, FILE_OPEN_MODE_READ_ONLY, 0);
    }

    return fd;
}