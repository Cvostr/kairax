#include "proc/syscalls.h"
#include "fs/vfs/vfs.h"
#include "errors.h"
#include "proc/process.h"
#include "mem/kheap.h"
#include "proc/thread_scheduler.h"
#include "mem/pmm.h"
#include "kstdlib.h"
#include "../elf_process_loader.h"
#include "string.h"
#include "dev/acpi/acpi.h"
#include "ipc/pipe.h"
#include "proc/timer.h"
#include "drivers/tty/tty.h"
#include "kairax/intctl.h"

#include "cpu/cpu_local_x64.h"

int sys_not_implemented()
{
    return -ERROR_WRONG_FUNCTION;
}

int sys_yield()
{
    struct thread* thr = cpu_get_current_thread();
    scheduler_yield(TRUE);
}

int sys_pipe(int* pipefd, int flags)
{
    struct process* process = cpu_get_current_thread()->process;
    VALIDATE_USER_POINTER(process, pipefd, sizeof(int*) * 2);

    struct pipe* ppe = new_pipe();
    if (ppe == NULL) {
        return -ENOMEM;
    }

    ppe->check_ends = 1; // EPIPE если другой конец закрыт

    struct file* pread_file = new_file();
    if (pread_file == NULL) {
        free_pipe(ppe);
        return -ENOMEM;  
    }
    
    struct file* pwrite_file = new_file();

    pipe_create_files(ppe, flags, pread_file, pwrite_file);

    pipefd[0] = process_add_file(process, pread_file);
    pipefd[1] = process_add_file(process, pwrite_file);

    return 0;
}

int sys_create_pty(int *master_fd, int *slave_fd)
{
    struct file *master;
    struct file *slave;
    int rc = tty_create(&master, &slave);

    struct process* process = cpu_get_current_thread()->process;
    *master_fd = process_add_file(process, master);
    *slave_fd = process_add_file(process, slave);

    return rc;
}

int sys_poweroff(int cmd)
{
    struct process* process = cpu_get_current_thread()->process;
    if (process->euid != 0) {
        return -EPERM;
    }

    switch (cmd) {
        case 0x30:
            acpi_poweroff();
            break;
        case 0x40:
            acpi_reboot();
            break;
        default:
            return -ERROR_INVALID_VALUE;
    }

    return -1;
}

int sys_thread_sleep(time_t sec, long int nsec)
{
    int rc = 0;

    if (sec == 0 && nsec == 0) {
        goto exit;
    }

    if (sec < 0 || nsec < 0) {
        rc = -ERROR_INVALID_VALUE;
        goto exit;
    }

    struct thread* thread = cpu_get_current_thread();
    struct timespec duration = {.tv_sec = sec, .tv_nsec = nsec};
    struct event_timer* timer = register_event_timer(duration);
    if (timer == NULL) {
        return -ENOMEM;
    }
    
    sleep_on_timer(timer);

    unregister_event_timer(timer);
    kfree(timer);

exit:
    return rc;
}

pid_t sys_create_thread(void* entry_ptr, void* arg, size_t stack_size)
{
    struct process* process = cpu_get_current_thread()->process;

    struct thread* thread = create_thread(process, entry_ptr, arg, stack_size, NULL);
    if (thread == NULL) {
        return -ENOMEM;
    }

    // Добавить в список и назначить pid
    process_add_to_list(thread);

    if (thread == NULL) {
        return -1;
    }

    if (stack_size > MAX_STACK_SIZE) 
    {
        stack_size = MAX_STACK_SIZE;
    }

    scheduler_add_thread(thread);

    return thread->id;
}

int sys_mount(const char* device, const char* mount_dir, const char* fs)
{
    drive_partition_t* partition = get_partition_with_name(device);

    if (partition == NULL) {
        return -1;
    }

    return vfs_mount_fs(mount_dir, partition, fs);
}

int sys_get_time_epoch_protected(struct timeval *tv)
{
    struct process* process = cpu_get_current_thread()->process;
    VALIDATE_USER_POINTER(process, tv, sizeof(struct timeval))

    return sys_get_time_epoch(tv);
}

int sys_get_time_epoch(struct timeval *tv)
{
    arch_sys_get_time_epoch(tv);
    return 0;
}

int sys_set_time_epoch(const struct timeval *tv)
{
    struct process* process = cpu_get_current_thread()->process;
    VALIDATE_USER_POINTER(process, tv, sizeof(struct timeval))

    if (tv->tv_sec < 0 || tv->tv_usec < 0 || tv->tv_usec > 1000000) {
        return -ERROR_INVALID_VALUE;
    }

    arch_sys_set_time_epoch(tv);
    return 0;
}

uint64_t sys_get_tick_count()
{
    struct timespec ticks;
    timer_get_ticks(&ticks);
    uint64_t t = ticks.tv_sec * 1000 + ticks.tv_nsec / 1000000;
    return t;
}

pid_t sys_create_process(int dirfd, const char* filepath, struct process_create_info* info)
{
    struct process* process = cpu_get_current_thread()->process;
    struct process* new_process = NULL;
    struct dentry* dir_dentry = NULL;
    int fd = -1;
    char* argv = NULL;
    int argc = 0;
    char interp_path[INTERP_PATH_MAX_LEN];
    void* program_start_ip = NULL;
    void* loader_start_ip = NULL;

    // Проверить количество аргументов
    if (info->num_args > PROCESS_MAX_ARGS) {
        // Слишком много аргументов, выйти с ошибкой
        goto exit;
    }

    // Получить dentry, относительно которой открыть файл
    fd = process_get_relative_direntry(process, dirfd, filepath, &dir_dentry);

    if (fd != 0) {
        // Не получилось найти директорию с файлом
        return fd;
    }

    // Открыть исполняемый файл
    struct file* file = file_open(dir_dentry, filepath, FILE_OPEN_MODE_READ_ONLY, 0);

    if (file == NULL) {
        // Файл не найден
        return -ERROR_NO_FILE;
    }

    if ( (file->inode->mode & INODE_TYPE_DIRECTORY)) {
        // Мы пытаемся открыть директорию - выходим
        file_close(file);
        return -ERROR_IS_DIRECTORY;
    }

    // Создать новый процесс
    new_process = create_new_process(process);
    strncpy(new_process->name, filepath, PROCESS_NAME_MAX_LEN);
    // Добавить в список и назначить pid
    process_add_to_list(new_process);

    // Процесс наследует ID пользователя
    new_process->uid = process->uid;
    new_process->euid = process->euid;
    new_process->gid = process->gid;
    new_process->egid = process->egid;

    // TEMPORARY
    struct file* stdout = process_get_file(process, info->stdout);
    if (stdout != NULL) {
        process_add_file_at(new_process, stdout, 1);
    }
    struct file* stdin = process_get_file(process, info->stdin);
    if (stdin != NULL) {
        process_add_file_at(new_process, stdin, 0);
    }
    struct file* stderr = process_get_file(process, info->stderr);
    if (stderr != NULL) {
        process_add_file_at(new_process, stderr, 2);
    }
    // -----------

    // Добавить файл к новому процессу
    fd = process_add_file(new_process, file);

    // Загрузка исполняемого файла
    size_t size = file->inode->size;
    char* image_data = kmalloc(size);
    ssize_t readed = file_read(file, size, image_data);
    if (readed < 0)
    {
        kfree(image_data);
        free_process(new_process);
        return readed;
    }

    // Попытаемся загрузить файл программы
    pid_t rc = elf_load_process(new_process, image_data, 0, &program_start_ip, interp_path);
    kfree(image_data);

    // Сбрасываем позицию файла чтобы загрузчик мог его прочитать еще раз
    file->pos = 0;
    // Файл не закрываем

    if (rc != 0) {
        // Ошибка загрузки
        // Файл будет закрыт при уничтожении объекта процесса
        free_process(new_process);
        return rc;
    }

    if (interp_path[0] == '\0') {
        strcpy(interp_path, "/loader.elf");
    }

    // Этап загрузки динамического линковщика
    struct file* loader_file = file_open(NULL, interp_path, FILE_OPEN_MODE_READ_ONLY, 0);
    if (loader_file == NULL) {
        // Загрузчик не найден
        free_process(new_process);
        return -ERROR_NO_FILE;
    }

    // Проверим, не является ли файл загрузчика директорией?
    if ( (loader_file->inode->mode & INODE_TYPE_DIRECTORY)) {
        // Мы пытаемся открыть директорию - выходим
        file_close(loader_file);
        free_process(new_process);
        return -ERROR_IS_DIRECTORY;
    }

    // Выделение памяти и чтение линковщика
    size = loader_file->inode->size;
    image_data = kmalloc(size);
    file_read(loader_file, size, image_data);
    file_close(loader_file);

    // Смещение в адресном пространстве процесса, по которому будет помещен загрузчик
    uint64_t loader_offset = new_process->brk + PAGE_SIZE;

    // Загрузить линковщик в адресное пространство процесса
    rc = elf_load_process(new_process, image_data, loader_offset, &loader_start_ip, NULL);
    kfree(image_data);

    if (rc != 0) {
        // Ошибка загрузки
        free_process(new_process);
        return rc;
    }

    if (info) {
            
        if (info->current_directory) {
            // Указана рабочая директория
            struct dentry* new_workdir = vfs_dentry_traverse_path(NULL, info->current_directory, 0);

            if (new_workdir) {
                
                // Проверить тип, убедиться что это директория
                if ((new_workdir->flags & DENTRY_TYPE_DIRECTORY) == DENTRY_TYPE_DIRECTORY) {
                    
                    dentry_open(new_workdir);
                    new_process->pwd = new_workdir;

                }
            }
        }

        // Загрузить аргументы в адресное пространство процесса
        argc = info->num_args;
        rc = process_load_arguments(new_process, info->num_args, info->args, &argv, 0);
        if (rc != 0) {
            free_process(new_process);
            goto exit;
        }
    }

    // У процесса так и нет рабочей папки
    // Используем папку родителя, если она есть
    if (new_process->pwd == NULL && process->pwd != NULL) {

        dentry_open(process->pwd);
        new_process->pwd = process->pwd;
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
    main_thr_info.argv = argv;
    main_thr_info.envp = NULL;

    // Создание главного потока
    struct thread* thr = create_thread(new_process, loader_start_ip, 0, 0, &main_thr_info);
	// Добавление потока в планировщик
    scheduler_add_thread(thr);
    process_add_to_list(thr);

    // Освободить память под aux
    kfree(aux_v);

    // Возвращаем pid процесса
    rc = new_process->pid;

exit:
    return rc;
}