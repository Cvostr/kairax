#include "proc/syscalls.h"
#include "fs/vfs/vfs.h"
#include "errors.h"
#include "proc/process.h"
#include "cpu/cpu_local_x64.h"
#include "mem/kheap.h"
#include "proc/thread_scheduler.h"
#include "mem/pmm.h"
#include "kstdlib.h"
#include "proc/elf64/elf64.h"
#include "string.h"
#include "dev/acpi/acpi.h"
#include "ipc/pipe.h"
#include "proc/timer.h"

int sys_not_implemented()
{
    return -ERROR_WRONG_FUNCTION;
}

int sys_pipe(int* pipefd, int flags)
{
    struct process* process = cpu_get_current_thread()->process;
    struct pipe* ppe = new_pipe();

    struct file* pread_file = new_file();
    struct file* pwrite_file = new_file();

    pipe_create_files(ppe, flags, pread_file, pwrite_file);

    pipefd[0] = process_add_file(process, pread_file);
    pipefd[1] = process_add_file(process, pwrite_file);

    return 0;
}

int sys_get_working_dir(char* buffer, size_t size)
{
    int rc = 0;
    size_t reqd_size = 0;
    struct process* process = cpu_get_current_thread()->process;

    if (process->workdir) {
        
        // Вычисляем необходимый размер буфера
        vfs_dentry_get_absolute_path(process->workdir->dentry, &reqd_size, NULL);
        
        if (reqd_size + 1 > size) {
            // Размер буфера недостаточный
            rc = -ERROR_RANGE;
            goto exit;
        }

        // Записываем путь в буфер
        vfs_dentry_get_absolute_path(process->workdir->dentry, NULL, buffer);
    }

exit:
    return rc;
}

int sys_set_working_dir(const char* buffer)
{
    int rc = 0;
    struct process* process = cpu_get_current_thread()->process;
    struct dentry* workdir_dentry = process->workdir != NULL ? process->workdir->dentry : NULL;
    struct file* new_workdir = file_open(workdir_dentry, buffer, 0, 0);

    if (new_workdir) {
        // файл открыт, убеждаемся что это директория 
        if (new_workdir->inode->mode & INODE_TYPE_DIRECTORY) {

            //  Закрываем старый файл, если был
            if (process->workdir) {
                file_close(process->workdir);
            }

            process->workdir = new_workdir;
        } else {
            // Это не директория
            rc = -ERROR_NOT_A_DIRECTORY;
            file_close(new_workdir);
        }
    } else {
        rc = -ERROR_NO_FILE;
    }

    return rc;
}

int sys_poweroff(int cmd)
{
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

pid_t sys_get_process_id()
{
    struct process* process = cpu_get_current_thread()->process;
    return process->pid;
}

pid_t sys_get_thread_id()
{
    struct thread* current_thread = cpu_get_current_thread();
    return current_thread->id;
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
    
    scheduler_sleep(timer, NULL);

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
        return -1;
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

void sys_exit_process(int code)
{
    struct process* process = cpu_get_current_thread()->process;
    // Данная операция должна выполниться атомарно
    disable_interrupts();
    // Сохранить код возврата
    process->code = code;
    // Очистить процесс, сделать его зомби
    process_become_zombie(process);
    // Разбудить потоки, ждущие pid
    scheduler_wakeup(process);
    // Удалить потоки процесса из планировщика
    scheduler_remove_process_threads(process);

    scheduler_yield(FALSE);
}

void sys_exit_thread(int code)
{
    // Данная операция должна выполниться атомарно
    disable_interrupts();
    // Получить объект потока
    struct thread* thr = cpu_get_current_thread();
    thr->code = code;
    thr->state = STATE_ZOMBIE;
    // Разбудить потоки, ждущие pid
    scheduler_wakeup(thr);
    // Убрать поток из списка планировщика
    scheduler_remove_thread(thr);
    // Выйти
    scheduler_yield(FALSE);
}

pid_t sys_wait(int mode, pid_t id, int* status, int options)
{
    pid_t       result = -1;
    struct process* process = cpu_get_current_thread()->process;

    acquire_spinlock(&process->wait_lock);
    if (mode == 0) {

        struct process* child = NULL;
        
        if (id > 0) {

            // pid указан, ищем процесс
            for (;;) {

                child = process_get_child_by_id(process, id);

                if (!child) 
                    goto exit;

                if (child->state == STATE_ZOMBIE)
                    break;

                scheduler_sleep(child, &process->wait_lock);
            }
        } else if (id == -1) {
            // Завершить любой дочерний процесс, который зомби
            // TODO
            
        } else {
            goto exit;
        }

        // Процесс завершен
        // Сохранить код вовзрата
        int code = child->code;
        // Удалить процесс из списка
        process_remove_from_list(child);
        // Удалить процесс из списка потомков
        process_remove_child(process, child);
        // Уничтожить объект процесса
        free_process(child);
        
        *status = code;
        result = id;
    }

exit:
    release_spinlock(&process->wait_lock);
    return result;
}

int sys_get_time_epoch(struct timeval *tv)
{
    arch_sys_get_time_epoch(tv);
    return 0;
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
    // Добавить в список и назначить pid
    process_add_to_list(new_process);

    // TEMPORARY
    struct file* stdout = process_get_file(process, info->stdout);
    if (stdout != NULL) {
        process_add_file_at(new_process, stdout, 1);
    }

    // Добавить файл к новому процессу
    fd = process_add_file(new_process, file);

    // Загрузка исполняемого файла
    size_t size = file->inode->size;
    char* image_data = kmalloc(size);
    file_read(file, size, image_data);

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
            struct file* new_workdir = file_open(NULL, info->current_directory, 0, 0);
            atomic_inc(&new_workdir->refs);

            if (new_workdir) {
                
                // Проверить тип inode, убедиться что это директория
                if (new_workdir->inode->mode & INODE_TYPE_DIRECTORY) {
                    
                    new_process->workdir = new_workdir;

                } else {
                    // Это не директория - закрываем файл
                    file_close(new_workdir);
                }
            }
        }

        // Загрузить аргументы в адресное пространство процесса
        argc = info->num_args;
        rc = process_load_arguments(new_process, info->num_args, info->args, &argv);
        if (rc != 0) {
            free_process(new_process);
            goto exit;
        }
    }

    // У процесса так и нет рабочей папки
    // Используем папку родителя, если она есть
    if (new_process->workdir == NULL && process->workdir != NULL) {

        new_process->workdir = file_clone(process->workdir);
        //new_process->workdir = process->workdir;
        atomic_inc(&new_process->workdir->refs);
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

    // Создание главного потока
    struct thread* thr = create_thread(new_process, loader_start_ip, 0, 0, &main_thr_info);
	// Добавление потока в планировщик
    scheduler_add_thread(thr);

    // Освободить память под aux
    kfree(aux_v);

    // Возвращаем pid процесса
    rc = new_process->pid;

exit:
    return rc;
}