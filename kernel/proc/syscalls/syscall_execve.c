#include "proc/syscalls.h"
#include "cpu/cpu_local.h"
#include "../elf_process_loader.h"
#include "mem/pmm.h"
#include "mem/kheap.h"
#include "kairax/string.h"

// X86
#include "proc/x64_context.h"
//

void free_twodim(char **arr, int size)
{
    int i;
    if (arr != NULL)
    {
        for (i = 0; i < size; i ++) 
        {
            if (arr[i])
                kfree(arr[i]);
        }

        kfree(arr);
    }
}

int sys_execve(const char *filepath, char *const argv [], char *const envp[])
{
    int rc;
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
    struct file* exec_file = NULL;
    struct file* loader_file = NULL;

    char** argvk = NULL;
    char** envpk = NULL;

    struct process* process = cpu_get_current_thread()->process;

    VALIDATE_USER_STRING(process, filepath)

    if (list_size(process->threads) > 1) {
        return -1;
    }

    if (list_size(process->children) > 0) {
        return -1;
    }

    // Заранее выделяем память под aux вектор
    struct aux_pair* aux_v = kmalloc(sizeof(struct aux_pair) * 4);
    if (aux_v == NULL) {
        return -ENOMEM;
    }

    if (argv != NULL) {
        VALIDATE_USER_POINTER(process, &argv[argc], sizeof(char*))
        while (argv[argc] != NULL) {
            argc++;
            summary_size += strlen(argv[argc]) + 1;
        }
        if (argc > PROCESS_MAX_ARGS) {
            // Слишком много аргументов, выйти с ошибкой
            rc = -E2BIG;
            goto error;
        }
    }

    // Проверка суммарного размера аргументов в памяти
    if (summary_size > PROCESS_MAX_ARGS_SIZE) {
        // Суммарный размер аргуменов большой, выйти с ошибкой
        rc = -ERROR_ARGS_BUFFER_BIG;
        goto error;
    }

    if (envp != NULL) {
        VALIDATE_USER_POINTER(process, &envp[envc], sizeof(char*))
        while (envp[envc] != NULL) {
            envc++;
        }
    }

    // Открыть исполняемый файл
    exec_file = file_open(process->pwd, filepath, FILE_OPEN_MODE_READ_ONLY, 0);
    if (exec_file == NULL) {
        // Файл не найден
        rc = -ERROR_NO_FILE;
        goto error;
    }
    if ( (exec_file->inode->mode & INODE_TYPE_DIRECTORY)) {
        // Мы пытаемся открыть директорию - выходим
        rc = -ERROR_IS_DIRECTORY;
        goto error;
    }
    if (inode_check_perm(exec_file->inode, process->euid, process->egid, S_IXUSR, S_IXGRP, S_IXOTH) == 0)
    {
        rc = -EACCES;
        goto error;
    }

    // Размер исполняемого файла
    fsize = exec_file->inode->size;
    if (fsize == 0)
    {
        rc = -ERROR_BAD_EXEC_FORMAT;
        goto error;
    }

    // Выделить память под данные исполняемого файла
    process_image_data = kmalloc(fsize);
    if (process_image_data == NULL) {
        rc = -ENOMEM;
        goto error;
    }
    
    // Чтение исполняемого файла
    ssize_t readed = file_read(exec_file, fsize, process_image_data);
    if (readed == 0 || readed != fsize)
    {
        rc = -ERROR_BAD_EXEC_FORMAT;
        goto error;
    }

    // Проверка заголовка
    struct elf_header* elf_header = (struct elf_header*) process_image_data;
    if (!elf_check_signature(elf_header)) 
    {
        rc = -ERROR_BAD_EXEC_FORMAT;
        goto error;
    }

    // Вытащить путь к загрузчику
    int need_ld = elf_get_loader_path(process_image_data, interp_path);

    // Нужен загрузчик
    if (interp_path[0] == '\0') {
        strcpy(interp_path, "/loader.elf");
    }

    // Пытаемся открыть файл динамического линковщика
    loader_file = file_open(NULL, interp_path, FILE_OPEN_MODE_READ_ONLY, 0);
    if (loader_file == NULL) {
        // Загрузчик не найден
        rc = -ERROR_NO_FILE;
        goto error;
    }
    // Проверим, не является ли файл линковщика директорией?
    if ( (loader_file->inode->mode & INODE_TYPE_DIRECTORY)) {
        // Мы пытаемся открыть директорию - выходим
        rc = -ERROR_IS_DIRECTORY;
        goto error;
    }
    if (inode_check_perm(loader_file->inode, process->euid, process->egid, S_IXUSR, S_IXGRP, S_IXOTH) == 0)
    {
        rc = -EACCES;
        goto error;
    }

    // Выделение памяти и чтение линковщика
    fsize = loader_file->inode->size;
    loader_image_data = kmalloc(fsize);
    if (loader_image_data == NULL) {
        rc = -ENOMEM;
        goto error;
    }
    
    file_read(loader_file, fsize, loader_image_data);
    file_close(loader_file);

    // Сохраняем значения аргументов в память ядра, т.к ниже мы уничтожим адресное пространство процесса
    argvk = kmalloc(argc * sizeof(char*));
    if (argvk == NULL) { rc = -ENOMEM; goto error; }
    for (i = 0; i < argc; i ++) 
    {
        argvk[i] = kmalloc(strlen(argv[i]) + 1);
        strcpy(argvk[i], argv[i]);
    }

    // Сохраняем значения ENV в память ядра, т.к ниже мы уничтожим адресное пространство процесса
    envpk = kmalloc(envc * sizeof(char*));
    if (envpk == NULL) { rc = -ENOMEM; goto error; }
    for (i = 0; i < envc; i ++) 
    {
        envpk[i] = kmalloc(strlen(envp[i]) + 1);
        strcpy(envpk[i], envp[i]);
    }

    // Меняем имя процесса, пока адресное пространство целое
    process_set_name(process, filepath);
    goto next;

error:
    free_twodim(argvk, argc);
    free_twodim(envpk, envc);
    if (loader_image_data) kfree(loader_image_data);
    if (loader_file) file_close(loader_file);
    if (process_image_data) kfree(process_image_data);
    if (exec_file) file_close(exec_file);

    return rc;
next:

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
            mmap_region_unref(range);
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

    // SETUID, SETGID
    if ((exec_file->inode->mode & INODE_MODE_SETUID) == INODE_MODE_SETUID)
    {
        process->uid = exec_file->inode->uid;
        process->euid = process->uid;
    }
    if ((exec_file->inode->mode & INODE_MODE_SETGID) == INODE_MODE_SETGID)
    {
        process->gid = exec_file->inode->gid;
        process->egid = process->gid;
    }

    // Обнуление указателя на вершину кучи
    process->brk = 0;

    // Добавить файл к процессу
    fd = process_add_file(process, exec_file);

    // Очистка информации о TLS
    kfree(process->tls);

    // Попытаемся загрузить файл программы
    rc = elf_load_process(process, process_image_data, 0, &program_start_ip, interp_path);
    kfree(process_image_data);

    // Сбрасываем позицию файла чтобы загрузчик мог его прочитать еще раз, но НЕ ЗАКРЫВАЕМ ЕГО
    exec_file->pos = 0;

    // Смещение в адресном пространстве процесса, по которому будет помещен загрузчик
    uint64_t loader_offset = process->brk + PAGE_SIZE;

    // Загрузить линковщик в адресное пространство процесса
    rc = elf_load_process(process, loader_image_data, loader_offset, &loader_start_ip, NULL);
    kfree(loader_image_data);

    char* argvm = NULL;
    char* envpm = NULL;
    if (argv) {
        // Загрузить аргументы в адресное пространство процесса
        rc = process_load_arguments(process, argc, argvk, &argvm, 1);
    }
    if (envp) {
        // Загрузить env в адресное пространство процесса с добавлением NULL на конце массива
        rc = process_load_arguments(process, envc, envpk, &envpm, 1);
    }

    // Формируем auxiliary вектор
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
	
    syscall_frame_t* syscall_frame = (syscall_frame_t*) this_core->kernel_stack;
    syscall_frame = syscall_frame - 1;

    syscall_frame->rcx = (uint64_t) loader_start_ip;
    set_user_stack_ptr(thr->stack_ptr);

    // Освободить память под aux
    kfree(aux_v);

    // Освободить память под временные двумерные массивы аргументов и окружения
    free_twodim(argvk, argc);
    free_twodim(envpk, envc);

    return 0;
}