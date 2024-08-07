#include "proc/syscalls.h"
#include "cpu/cpu_local_x64.h"
#include "../elf_process_loader.h"
#include "mem/pmm.h"
#include "mem/kheap.h"
#include "kairax/string.h"

// X86
#include "proc/x64_context.h"
//

int sys_execve(const char *filepath, char *const argv [], char *const envp[])
{
    int fd;
    int argc = 0;
    int envc = 0;
    int i;
    size_t fsize;
    size_t summary_size = 0;
    char* process_image_data = NULL;
    char* loader_image_data = NULL;
    char interp_path[INTERP_PATH_MAX_LEN];
    void* program_start_ip = NULL;
    void* loader_start_ip = NULL;

    struct process* process = cpu_get_current_thread()->process;

    if (list_size(process->threads) > 1) {
        return -1;
    }

    if (list_size(process->children) > 0) {
        return -1;
    }

    if (argv != NULL) {
        while (argv[argc] != NULL) {
            argc++;
            summary_size += strlen(argv[argc]) + 1;
        }
        if (argc > PROCESS_MAX_ARGS) {
            // Слишком много аргументов, выйти с ошибкой
            return -E2BIG;
        }
    }

    // Проверка суммарного размера аргументов в памяти
    if (summary_size > PROCESS_MAX_ARGS_SIZE) {
        // Суммарный размер аргуменов большой, выйти с ошибкой
        return -ERROR_ARGS_BUFFER_BIG;
    }

    if (envp != NULL) {
        while (envp[envc] != NULL) {
            envc++;
        }
    }

    // Открыть исполняемый файл
    struct file* file = file_open(process->pwd, filepath, FILE_OPEN_MODE_READ_ONLY, 0);

    if (file == NULL) {
        // Файл не найден
        return -ERROR_NO_FILE;
    }
    if ( (file->inode->mode & INODE_TYPE_DIRECTORY)) {
        // Мы пытаемся открыть директорию - выходим
        file_close(file);
        return -ERROR_IS_DIRECTORY;
    }
    //TODO: проверить атрибут исполнения

    // Чтение исполняемого файла
    fsize = file->inode->size;
    process_image_data = kmalloc(fsize);
    if (process_image_data == NULL) {
        file_close(file);
        return -ENOMEM;
    }
    file_read(file, fsize, process_image_data);

    // Проверка заголовка
    struct elf_header* elf_header = (struct elf_header*) process_image_data;
    if (!elf_check_signature(elf_header)) 
    {
        kfree(process_image_data);
        file_close(file);
        return -ERROR_BAD_EXEC_FORMAT;
    }

    // Вытащить путь к загрузчику
    int need_ld = elf_get_loader_path(process_image_data, interp_path);

    // Нужен загрузчик
    if (interp_path[0] == '\0') {
        strcpy(interp_path, "/loader.elf");
    }

    // Пытаемся открыть файл динамического линковщика
    struct file* loader_file = file_open(NULL, interp_path, FILE_OPEN_MODE_READ_ONLY, 0);
    if (loader_file == NULL) {
        // Загрузчик не найден
        kfree(process_image_data);
        file_close(file);
        return -ERROR_NO_FILE;
    }
    // Проверим, не является ли файл линковщика директорией?
    if ( (loader_file->inode->mode & INODE_TYPE_DIRECTORY)) {
        // Мы пытаемся открыть директорию - выходим
        kfree(process_image_data);
        file_close(loader_file);
        file_close(file);
        return -ERROR_IS_DIRECTORY;
    }
    // Выделение памяти и чтение линковщика
    fsize = loader_file->inode->size;
    loader_image_data = kmalloc(fsize);
    if (loader_image_data == NULL) {
        kfree(process_image_data);
        file_close(loader_file);
        file_close(file);
        return -ENOMEM;
    }
    file_read(loader_file, fsize, loader_image_data);
    file_close(loader_file);

    // Сохраняем значения аргументов в память ядра, т.к ниже мы уничтожим адресное пространство процесса
    char** argvk = kmalloc(argc * sizeof(char*));
    for (i = 0; i < argc; i ++) {
        argvk[i] = kmalloc(strlen(argv[i]) + 1);
        strcpy(argvk[i], argv[i]);
    }

    // Сохраняем значения ENV в память ядра, т.к ниже мы уничтожим адресное пространство процесса
    char** envpk = kmalloc(envc * sizeof(char*));
    for (i = 0; i < envc; i ++) {
        envpk[i] = kmalloc(strlen(envp[i]) + 1);
        strcpy(envpk[i], envp[i]);
    }

    // Меняем имя процесса, пока адресное пространство целое
    strncpy(process->name, filepath, PROCESS_NAME_MAX_LEN);

    // ------------ ТОЧКА НЕВОЗВРАТА --------------------
    // Все необходимые проверки должны быть сделаны до этого момента, потому что потом возвращаться будет некуда
    // Уничтожение неразделяемых регионов адресного пространства процесса
    struct list_node* current_area_node = process->mmap_ranges->head;
    struct list_node* next = current_area_node;

    while (current_area_node != NULL) 
    {
        next = next->next;

        struct mmap_range* range = (struct mmap_range*) current_area_node->element;

        // Является ли регион разделяемым?
        int shared = (range->flags & MAP_SHARED) == MAP_SHARED;

        // Сносим все регионы, которые не являются разделяемыми
        if (!shared) {
            list_remove(process->mmap_ranges, range);
            vm_table_unmap_region(process->vmemory_table, range->base, range->length);
            kfree(range);
        }

        current_area_node = next;
    }

    // Закрыть все файловые дескрипторы с флагом CLOSE ON EXEC
    for (i = 0; i < MAX_DESCRIPTORS; i ++)
    {
        if (process->fds[i] != NULL) {

            if (process_get_cloexec(process, i) == 1) {
                process_close_file(process, i);
            }
        }
    }

    // Обнуление указателя на вершину кучи
    process->brk = 0;

    // Добавить файл к процессу
    fd = process_add_file(process, file);

    // Очистка информации о TLS
    kfree(process->tls);

    // Попытаемся загрузить файл программы
    pid_t rc = elf_load_process(process, process_image_data, 0, &program_start_ip, interp_path);
    kfree(process_image_data);

    // Сбрасываем позицию файла чтобы загрузчик мог его прочитать еще раз, но НЕ ЗАКРЫВАЕМ ЕГО
    file->pos = 0;

    // Смещение в адресном пространстве процесса, по которому будет помещен загрузчик
    uint64_t loader_offset = process->brk + PAGE_SIZE;

    // Загрузить линковщик в адресное пространство процесса
    rc = elf_load_process(process, loader_image_data, loader_offset, &loader_start_ip, NULL);
    kfree(loader_image_data);


    char* argvm = NULL;
    char* envpm = NULL;
    if (argv) {
        // Загрузить аргументы в адресное пространство процесса
        rc = process_load_arguments(process, argc, argvk, &argvm, 0);
    }
    if (envp) {
        // Загрузить env в адресное пространство процесса с добавлением NULL на конце массива
        rc = process_load_arguments(process, envc, envpk, &envpm, 1);
    }

    // Формируем auxiliary вектор
    struct aux_pair* aux_v = kmalloc(sizeof(struct aux_pair) * 4);
    aux_v[1].type = AT_EXECFD;
    aux_v[1].ival = fd;
    aux_v[2].type = AT_ENTRY;
    aux_v[2].pval = program_start_ip;
    aux_v[3].type = AT_PAGESZ;
    aux_v[3].ival = PAGE_SIZE;
    aux_v[0].type = AT_NULL;

    struct main_thread_create_info main_thr_info;
    main_thr_info.auxv = aux_v;
    main_thr_info.aux_size = 4;
    main_thr_info.argc = argc;
    main_thr_info.argv = argvm;
    main_thr_info.envp = envpm;

    // Пересоздание главного потока
    struct thread* thr = process->threads->head->element;
    thread_recreate_on_execve(thr, &main_thr_info);
	
    syscall_frame_t* syscall_frame = this_core->kernel_stack;
    syscall_frame = syscall_frame - 1;

    syscall_frame->rcx = loader_start_ip;
    set_user_stack_ptr(thr->stack_ptr);

    // Освободить память под aux
    kfree(aux_v);

    for (i = 0; i < argc; i ++) {
        kfree(argvk[i]);
    }
    kfree(argvk);

    for (i = 0; i < envc; i ++) {
        kfree(envpk[i]);
    }
    kfree(envpk);

    return 0;
}