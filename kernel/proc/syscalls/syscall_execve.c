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

    while (argv[argc] != NULL) {
        argc++;
    }

    while (envp[envc] != NULL) {
        envc++;
    }

    // Сохраняем значения аргументов в память ядра, т.к ниже мы уничтожим адресное пространство процесса
    char** argvk = kmalloc(argc * sizeof(char*));
    for (i = 0; i < argc; i ++) {
        argvk[i] = kmalloc(strlen(argv[i]) + 1);
        strcpy(argvk[i], argv[i]);
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

    // Чтение исполняемого файла
    size_t size = file->inode->size;
    char* image_data = kmalloc(size);
    file_read(file, size, image_data);

    // Проверка заголовка
    struct elf_header* elf_header = (struct elf_header*) image_data;
    if (!elf_check_signature(elf_header)) 
    {
        kfree(image_data);
        file_close(file);
        return -ERROR_BAD_EXEC_FORMAT;
    }

    // TODO: копирование ENV

    // Уничтожение неразделяемых регионов адресного пространства процесса
    struct list_node* current_area_node = process->mmap_ranges->head;
    struct list_node* next = current_area_node;

    while (current_area_node != NULL) 
    {
        next = next->next;

        struct mmap_range* range = (struct mmap_range*) current_area_node->element;

        // Является ли регион разделяемым?
        int is_shared = (range->flags & MAP_SHARED) == MAP_SHARED;

        // Сносим все регионы, которые не являются разделяемыми
        if (!is_shared) {
            list_remove(process->mmap_ranges, range);
            vm_table_unmap_region(process->vmemory_table, range->base, range->length);
            kfree(range);
        }

        current_area_node = next;
    }

    // Обнуление указателя на вершину кучи
    process->brk = 0;

    // Добавить файл к процессу
    fd = process_add_file(process, file);

    // Очистка информации о TLS
    kfree(process->tls);

    // Попытаемся загрузить файл программы
    pid_t rc = elf_load_process(process, image_data, 0, &program_start_ip, interp_path);
    kfree(image_data);

    // Сбрасываем позицию файла чтобы загрузчик мог его прочитать еще раз
    file->pos = 0;
    // Файл не закрываем

    if (rc != 0) {
        // Ошибка загрузки
        file_close(file);
        return rc;
    }

    if (interp_path[0] == '\0') {
        strcpy(interp_path, "/loader.elf");
    }

    // Этап загрузки динамического линковщика
    struct file* loader_file = file_open(NULL, interp_path, FILE_OPEN_MODE_READ_ONLY, 0);
    if (loader_file == NULL) {
        // Загрузчик не найден
        file_close(file);
        return -ERROR_NO_FILE;
    }

    // Проверим, не является ли файл загрузчика директорией?
    if ( (loader_file->inode->mode & INODE_TYPE_DIRECTORY)) {
        // Мы пытаемся открыть директорию - выходим
        file_close(loader_file);
        file_close(file);
        return -ERROR_IS_DIRECTORY;
    }

    // Выделение памяти и чтение линковщика
    size = loader_file->inode->size;
    image_data = kmalloc(size);
    file_read(loader_file, size, image_data);
    file_close(loader_file);

    // Смещение в адресном пространстве процесса, по которому будет помещен загрузчик
    uint64_t loader_offset = process->brk + PAGE_SIZE;

    // Загрузить линковщик в адресное пространство процесса
    rc = elf_load_process(process, image_data, loader_offset, &loader_start_ip, NULL);
    kfree(image_data);

    if (rc != 0) {
        // Ошибка загрузки
        //free_process(new_process);
        return rc;
    }

    char* argvm = NULL;
    if (argv) {
            
        // Загрузить аргументы в адресное пространство процесса
        rc = process_load_arguments(process, argc, argvk, &argvm);
        if (rc != 0) {
            //free_process(new_process);
            //goto exit;
        }
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

}