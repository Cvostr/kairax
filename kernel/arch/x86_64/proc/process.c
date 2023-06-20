#include "memory/kernel_vmm.h"
#include "mem/pmm.h"
#include "mem/kheap.h"
#include "string.h"
#include "memory/mem_layout.h"
#include "proc/elf64/elf64.h"
#include "proc/process.h"
#include "thread.h"
#include "thread_scheduler.h"
#include "fs/vfs/vfs.h"

int last_pid = 0;

process_t*  create_new_process()
{
    process_t* process = (process_t*)kmalloc(sizeof(process_t));
    memset(process, 0, sizeof(process_t));

    process->name[0] = '\0';
    // Склонировать таблицу виртуальной памяти ядра
    process->vmemory_table = vmm_clone_kernel_memory_map();
    process->brk = 0x0;
    process->pid = last_pid++;
    // Создать список потоков
    process->threads = create_list();

    return process;
}

int create_new_process_from_image(char* image)
{
    elf_header_t* elf_header = (elf_header_t*)image;

    if(elf_check_signature(elf_header)) {
        //Это ELF файл
        //Создаем объект процесса
        process_t* proc = create_new_process();

        elf_sections_ptr_t sections_ptrs;
        elf_read_sections(image, &sections_ptrs);

        for (uint32_t i = 0; i < elf_header->prog_header_entries_num; i ++) {
            elf_program_header_entry_t* pehentry = elf_get_program_entry(image, i);
            /*printf("PE: foffset %i, fsize %i, vaddr %i, memsz %i, type %i\n",
                pehentry->p_offset,
                pehentry->p_filesz,
                pehentry->v_addr,
                pehentry->p_memsz,
                pehentry->type);*/

            // Выделить память в виртуальной таблице процесса 
            process_alloc_memory(proc, pehentry->v_addr, pehentry->p_memsz, PAGE_USER_ACCESSIBLE | PAGE_WRITABLE | PAGE_PRESENT);
            // Заполнить выделенную память нулями
            memset_vm(proc->vmemory_table, pehentry->v_addr, 0, pehentry->p_memsz);
            // Копировать фрагмент программы в память
            copy_to_vm(proc->vmemory_table, pehentry->v_addr, image + pehentry->p_offset, pehentry->p_filesz);   
        }

        // Создание главного потока и передача выполнения
        thread_t* thr = create_thread(proc, elf_header->prog_entry_pos);
	    scheduler_add_thread(thr);  
        
    } else {
        return -1;
    }
}

uintptr_t process_brk_flags(process_t* process, void* addr, uint64_t flags)
{
    uintptr_t uaddr = addr;
    //Выравнивание до размера страницы в большую сторону
    uaddr += (PAGE_SIZE - (uaddr % PAGE_SIZE));
    //До тех пор, пока адрес конца памяти процесса меньше, выделяем страницы
    while((uint64_t)uaddr > process->brk) {
        map_page_mem(process->vmemory_table, process->brk, pmm_alloc_page(), flags);
        process->brk += PAGE_SIZE;
    }

    //До тех пор, пока адрес конца памяти процесса больше, освобождаем страницы
    while((uint64_t)uaddr < process->brk) {
        uintptr_t virtual_page_start = process->brk - PAGE_SIZE;
        physical_addr_t phys_addr = get_physical_address(process->vmemory_table, virtual_page_start);
        unmap_page(process->vmemory_table, virtual_page_start);
        pmm_free_page(phys_addr);
        process->brk -= PAGE_SIZE;
    }

    return process->brk;
}

uintptr_t process_brk(process_t* process, void* addr)
{
    return process_brk_flags(process, addr, PAGE_USER_ACCESSIBLE | PAGE_WRITABLE | PAGE_PRESENT);
}

int process_alloc_memory(process_t* process, uintptr_t start, uintptr_t size, uint64_t flags)
{
    uintptr_t start_aligned = start - (start % PAGE_SIZE); // выравнивание в меньшую сторону
    uintptr_t size_aligned = size + (PAGE_SIZE - (size % PAGE_SIZE)); //выравнивание в большую сторону

    for (uintptr_t page_i = start_aligned; page_i < start_aligned + size_aligned; page_i += PAGE_SIZE) {
        map_page_mem(process->vmemory_table, page_i, pmm_alloc_page(), flags);

        if (page_i > process->brk)
            process->brk = page_i + PAGE_SIZE;
    }
}

int process_open_file(process_t* process, const char* path, int mode, int flags)
{
    int fd = -1;
    vfs_inode_t* inode = vfs_fopen(path, 0);

    // Создать новый дескриптор файла
    file_t* file = kmalloc(sizeof(file_t));
    file->inode = inode;
    file->mode = mode;
    file->flags = flags;
    file->pos = 0;

    // Найти свободный номер дескриптора для процесса
    acquire_spinlock(&process->fd_spinlock);
    for (int i = 0; i < MAX_DESCRIPTORS; i ++) {
        if (process->fds[i] == NULL) {
            process->fds[i] = file;
            fd = i;
            goto exit;
        }
    }

    // Не получилось привязать дескриптор к процессу, освободить память под file
    kfree(file);

exit:
    release_spinlock(&process->fd_spinlock);
    return fd;
}

int process_close_file(process_t* process, int fd)
{
    acquire_spinlock(&process->fd_spinlock);
    file_t* file = process->fds[fd];
    if (file != NULL) {
        vfs_close(file->inode);
        kfree(file);
        process->fds[fd];
    }

    release_spinlock(&process->fd_spinlock);
    return 0;
}

size_t process_read_file(process_t* process, int fd, char* buffer, size_t size)
{
    size_t bytes_read = 0;
    acquire_spinlock(&process->fd_spinlock);

    file_t* file = process->fds[fd];
    if (file->mode & FILE_OPEN_MODE_READ_ONLY) {
        vfs_inode_t* inode = file->inode;
        bytes_read = vfs_read(inode, file->pos, size, buffer);
        file->pos += bytes_read; 
    }

    release_spinlock(&process->fd_spinlock);

    return bytes_read;
}