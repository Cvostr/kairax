#include "syscalls.h"
#include "fs/vfs/vfs.h"
#include "errors.h"
#include "process.h"
#include "cpu/cpu_local_x64.h"
#include "mem/kheap.h"
#include "proc/thread_scheduler.h"
#include "mem/pmm.h"
#include "kstdlib.h"
#include "elf64/elf64.h"
#include "string.h"
#include "dev/acpi/acpi.h"
#include "ipc/pipe.h"

int sys_not_implemented()
{
    return -ERROR_WRONG_FUNCTION;
}

int sys_open_file(int dirfd, const char* path, int flags, int mode)
{
    int fd = -1;
    struct dentry* dir_dentry = NULL;
    struct process* process = cpu_get_current_thread()->process;

    // Получить dentry, относительно которой открыть файл
    fd = process_get_relative_direntry(process, dirfd, path, &dir_dentry);

    if (fd != 0) {
        // Не получилось найти директорию с файлом
        return fd;
    }

    // Открыть файл в ядре
    struct file* file = file_open(dir_dentry, path, flags, mode);

    if (file == NULL) {
        // Файл не найден
        return -ERROR_NO_FILE;
    }

    if ( !(file->inode->mode & INODE_TYPE_DIRECTORY) && (flags & FILE_OPEN_FLAG_DIRECTORY)) {
        // Мы пытаемся открыть обычный файл с флагом директории - выходим
        file_close(file);
        return -ERROR_NOT_A_DIRECTORY;
    }

    if ((file->inode->mode & INODE_TYPE_DIRECTORY) && file_allow_write(file)) {
        // Мы пытаемся открыть директорию с правами на запись
        file_close(file);
        return -ERROR_IS_DIRECTORY;
    }

    // Добавить файл к процессу
    fd = process_add_file(process, file);

    if (fd < 0) {
        // Не получилось привязать дескриптор к процессу, закрыть файл
        file_close(file);
    }

    return fd;
}


int sys_close_file(int fd)
{
    struct process* process = cpu_get_current_thread()->process;
    return process_close_file(process, fd);
}

int sys_mkdir(int dirfd, const char* path, int mode)
{
    int rc = -1;
    struct dentry* dir_dentry = NULL;
    struct process* process = cpu_get_current_thread()->process;

    // Получить dentry, относительно которой создать папку
    rc = process_get_relative_direntry(process, dirfd, path, &dir_dentry);
    if (rc != 0) {
        return rc;
    }

    char* directory_path = NULL;
    char* filename = NULL;

    // Разделить путь на путь директории имя файла
    split_path(path, &directory_path, &filename);

    // Открываем inode директории
    struct inode* dir_inode = vfs_fopen(dir_dentry, directory_path, 0, NULL);

    if (directory_path) {
        kfree(directory_path);
    }

    if (dir_inode == NULL) {
        // Отсутствует полный путь к директории
        return -1;
    }

    // Создать папку
    rc = inode_mkdir(dir_inode, filename, mode);
    // Закрыть inode директории
    inode_close(dir_inode);

    return rc;
}

ssize_t sys_read_file(int fd, char* buffer, size_t size)
{
    ssize_t bytes_read = -1;
    struct process* process = cpu_get_current_thread()->process;
    struct file* file = process_get_file(process, fd);

    if (file == NULL) {
        bytes_read = -ERROR_BAD_FD;
        goto exit;
    }

    // Чтение из файла
    bytes_read = file_read(file, size, buffer);
exit:
    return bytes_read;
}

ssize_t sys_write_file(int fd, const char* buffer, size_t size)
{
    ssize_t bytes_written = -1;
    struct process* process = cpu_get_current_thread()->process;
    struct file* file = process_get_file(process, fd);

    if (file == NULL) {
        bytes_written = -ERROR_BAD_FD;
        goto exit;
    }
    
    // Записать в файл
    bytes_written = file_write(file, size, buffer);

exit:
    return bytes_written;
}

int sys_stat(int dirfd, const char* filepath, struct stat* statbuf, int flags)
{
    int rc = -1;
    int close_at_end = 0;
    struct process* process = cpu_get_current_thread()->process;
    struct file* file = NULL;

    // Открыть файл относительно папки и флагов
    rc = process_open_file_relative(process, dirfd, filepath, flags, &file, &close_at_end);
    if (rc != 0) {
        return rc;
    }

    struct inode* inode = file->inode;
    rc = inode_stat(inode, statbuf);

    if (close_at_end) {
        file_close(file);
    }

    return rc;
}

int sys_set_mode(int dirfd, const char* filepath, mode_t mode, int flags)
{
    int rc = -1;
    int close_at_end = 0;
    struct process* process = cpu_get_current_thread()->process;
    struct file* file = NULL;
    
    // Открыть файл относительно папки и флагов
    rc = process_open_file_relative(process, dirfd, filepath, flags, &file, &close_at_end);
    if (rc != 0) {
        return rc;
    }

    struct inode* inode = file->inode;
    rc = inode_chmod(inode, mode);

    if (close_at_end) {
        file_close(file);
    }

    return rc;
}

int sys_readdir(int fd, struct dirent* dirent)
{
    int rc = -1;
    struct process* process = cpu_get_current_thread()->process;
    struct file* file = process_get_file(process, fd);

    if (file != NULL) {
        // Вызов readdir из файла   
        rc = file_readdir(file, dirent);
    } else {
        
        rc = -ERROR_BAD_FD;
    }

exit:

    return rc;
}

int sys_ioctl(int fd, uint64_t request, uint64_t arg)
{
    int rc = -1;
    struct process* process = cpu_get_current_thread()->process;

    struct file* file = process_get_file(process, fd);

    if (file != NULL) {
        rc = file_ioctl(file, request, arg);
    } else {
        rc = -ERROR_BAD_FD;
    }

    return rc;
}

off_t sys_file_seek(int fd, off_t offset, int whence)
{
    off_t result = -1;
    struct process* process = cpu_get_current_thread()->process;
    struct file* file = process_get_file(process, fd);

    if (file != NULL) {
        result = file_seek(file, offset, whence);
    }

    return result;
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

void sys_exit_process(int code)
{
    struct process* process = cpu_get_current_thread()->process;
    scheduler_remove_process_threads(process);
    free_process(process);
    scheduler_from_killed();
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

int sys_thread_sleep(uint64_t time)
{
    struct thread* thread = cpu_get_current_thread();
    thread->state = THREAD_UNINTERRUPTIBLE; // Ожидающий системный вызов
    for (uint64_t i = 0; i < time; i ++) {
        scheduler_yield();
    }
    thread->state = THREAD_RUNNING;
}

int sys_create_thread(void* entry_ptr, void* arg, pid_t* tid, size_t stack_size)
{
    struct process* process = cpu_get_current_thread()->process;
    struct thread* thread = create_thread(process, entry_ptr, arg, NULL, stack_size, NULL);

    if (thread == NULL) {
        return -1;
    }

    if (tid != NULL) {
        *tid = thread->id;
    }

    scheduler_add_thread(thread);

    return 0;
}

void* sys_memory_map(void* address, uint64_t length, int protection, int flags)
{
    printf("ADDR %s ", ulltoa(address, 16));
    printf("SZ %s", ulltoa(length, 16));
    struct process* process = cpu_get_current_thread()->process;

    if (length == 0) {
        return (void*)-ERROR_INVALID_VALUE;
    }

    length = align(length, PAGE_SIZE);

    // Получить предпочитаемый адрес
    address = process_get_free_addr(process, length, align_down(address, PAGE_SIZE));

    // Сформировать регион
    struct mmap_range* range = kmalloc(sizeof(struct mmap_range));
    range->base = (uint64_t) address;
    range->length = length;
    range->protection = protection | PAGE_PROTECTION_USER;

    process_add_mmap_region(process, range);

    //NOT IMPLEMENTED FOR FILE MAP
    //TODO: IMPLEMENT

    printf(" FOUND ADDR %s\n", ulltoa(address, 16));
    return address;
}

int sys_memory_unmap(void* address, uint64_t length)
{
    // ONLY implemented unmapping by address
    // TODO : implement splitting
    struct process* process = cpu_get_current_thread()->process;
    int rc = 0;

    acquire_spinlock(&process->mmap_lock);
    struct mmap_range* region = process_get_range_by_addr(process, address);
    
    if (region == NULL) {
        rc = -1; //Уточнить код ошибки
        goto exit;
    }

    // TODO: reimplement
    length = region->length;

    // Удалить регион из списка
    list_remove(process->mmap_ranges, region);

    // Освободить память
    vm_table_unmap_region(process->vmemory_table, address, length);

    kfree(region);

exit:
    release_spinlock(&process->mmap_lock);
    return rc;
}

int sys_memory_protect(void* address, uint64_t length, int protection)
{
    return -1;
}

int sys_mount(const char* device, const char* mount_dir, const char* fs)
{
    drive_partition_t* partition = get_partition_with_name(device);

    if (partition == NULL) {
        return -1;
    }

    return vfs_mount_fs(mount_dir, partition, fs);
}

int sys_create_process(int dirfd, const char* filepath, struct process_create_info* info)
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

    // Добавить файл к новому процессу
    fd = process_add_file(new_process, file);

    // Загрузка исполняемого файла
    size_t size = file->inode->size;
    char* image_data = kmalloc(size);
    file_read(file, size, image_data);

    // Попытаемся загрузить файл программы
    int rc = elf_load_process(new_process, image_data, 0, &program_start_ip, interp_path);
    kfree(image_data);

    // Сбрасываем позицию файла чтобы загрузчик мог его прочитать еще раз
    file->pos = 0;
    // Файл не закрываем
    //file_close(file);

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
    struct thread* thr = create_thread(new_process, loader_start_ip, 0, 0, 0, &main_thr_info);
	// Добавление потока в планировщик
    scheduler_add_thread(thr);

    kfree(aux_v);

exit:
    return rc;
}