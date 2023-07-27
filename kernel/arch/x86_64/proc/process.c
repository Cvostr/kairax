#include "memory/kernel_vmm.h"
#include "mem/pmm.h"
#include "mem/kheap.h"
#include "string.h"
#include "memory/mem_layout.h"
#include "proc/elf64/elf64.h"
#include "proc/process.h"
#include "proc/thread.h"
#include "proc/thread_scheduler.h"
#include "fs/vfs/vfs.h"
#include "kstdlib.h"

int last_pid = 0;

struct process*  create_new_process(struct process* parent)
{
    struct process* process = (struct process*)kmalloc(sizeof(struct process));
    memset(process, 0, sizeof(struct process));

    process->name[0] = '\0';
    // Склонировать таблицу виртуальной памяти ядра
    process->vmemory_table = vmm_clone_kernel_memory_map();
    process->brk = 0x0;
    process->pid = last_pid++;
    process->parent = parent;
    // Создать список потоков
    process->threads = create_list();
    process->children = create_list();

    return process;
}

int create_new_process_from_image(struct process* parent, char* image, struct process_create_info* info)
{
    elf_header_t* elf_header = (elf_header_t*)image;

    if(elf_check_signature(elf_header)) {
        //Это ELF файл
        //Создаем объект процесса
        struct process* proc = create_new_process(parent);

        elf_sections_ptr_t sections_ptrs;
        // Считать таблицу секций
        elf_read_sections(image, &sections_ptrs);

        for (uint32_t i = 0; i < elf_header->prog_header_entries_num; i ++) {
            elf_program_header_entry_t* pehentry = elf_get_program_entry(image, i);
            /*printf("PE: foffset %i, fsize %i, vaddr %i, memsz %i, align %i, type %i\n",
                pehentry->p_offset,
                pehentry->p_filesz,
                pehentry->v_addr,
                pehentry->p_memsz,
                pehentry->alignment,
                pehentry->type);*/

            size_t aligned_size = align(pehentry->p_memsz, pehentry->alignment);

            if (pehentry->type != 7) {
                // Выделить память в виртуальной таблице процесса 
                process_alloc_memory(proc, pehentry->v_addr, aligned_size, PAGE_USER_ACCESSIBLE | PAGE_WRITABLE | PAGE_PRESENT);
                // Заполнить выделенную память нулями
                memset_vm(proc->vmemory_table, pehentry->v_addr, 0, aligned_size);
                // Копировать фрагмент программы в память
                copy_to_vm(proc->vmemory_table, pehentry->v_addr, image + pehentry->p_offset, pehentry->p_filesz);   
            }
        }

        if (sections_ptrs.tbss_ptr) {
            // Есть секция TLS BSS
            elf_section_header_entry_t* tbss_ptr = sections_ptrs.tbss_ptr;
            proc->tls = kmalloc(tbss_ptr->size);
            memset(proc->tls, 0, tbss_ptr->size);
            proc->tls_size = tbss_ptr->size;
        } 
        else if (sections_ptrs.tdata_ptr) {
            // Есть секция TLS DATA
            elf_section_header_entry_t* tdata_ptr = sections_ptrs.tdata_ptr;
            proc->tls = kmalloc(tdata_ptr->size);
            memcpy(proc->tls, image + tdata_ptr->offset, tdata_ptr->size);
            proc->tls_size = tdata_ptr->size;
        }

        if (info) {
            if (info->current_directory) {
                // Указана папка
                proc->cwd_inode = vfs_fopen(NULL, info->current_directory, 0, &proc->cwd_dentry);
            } else {
                // TODO: Используем папку родителя
            }

            /*size_t args_array_size = sizeof(char*) * info->num_args;
            size_t reqd_size = args_array_size;
            for (size_t i = 0; i < args_array_size; i ++) {
                reqd_size += strlen(info->args[i]);
            }*/
            
            //char* args_mem = (char*)process_brk(proc, proc->brk + reqd_size) - reqd_size;
        }

        // Создание главного потока и передача выполнения
        struct thread* thr = create_thread(proc, (void*)elf_header->prog_entry_pos, NULL, 0);
	    scheduler_add_thread(thr);  
        
    } else {
        return -1;
    }
}

uintptr_t process_brk_flags(struct process* process, void* addr, uint64_t flags)
{
    uintptr_t uaddr = (uintptr_t)addr;
    //Выравнивание до размера страницы в большую сторону
    uaddr += (PAGE_SIZE - (uaddr % PAGE_SIZE));
    //До тех пор, пока адрес конца памяти процесса меньше, выделяем страницы
    while((uint64_t)uaddr > process->brk) {
        map_page_mem(process->vmemory_table, process->brk, (physical_addr_t)pmm_alloc_page(), flags);
        process->brk += PAGE_SIZE;
    }

    //До тех пор, пока адрес конца памяти процесса больше, освобождаем страницы
    /*while((uint64_t)uaddr < process->brk) {
        uintptr_t virtual_page_start = process->brk - PAGE_SIZE;
        physical_addr_t phys_addr = get_physical_address(process->vmemory_table, virtual_page_start);
        unmap_page(process->vmemory_table, virtual_page_start);
        pmm_free_page((void*)phys_addr);
        process->brk -= PAGE_SIZE;
    }*/

    return process->brk;
}

uintptr_t process_brk(struct process* process, uint64_t addr)
{
    return process_brk_flags(process, (void*)addr, PAGE_USER_ACCESSIBLE | PAGE_WRITABLE | PAGE_PRESENT);
}

int process_alloc_memory(struct process* process, uintptr_t start, uintptr_t size, uint64_t flags)
{
    uintptr_t start_aligned = align_down(start, PAGE_SIZE); // выравнивание в меньшую сторону
    uintptr_t size_aligned = align(size, PAGE_SIZE); //выравнивание в большую сторону

    for (uintptr_t page_i = start_aligned; page_i < start_aligned + size_aligned; page_i += PAGE_SIZE) {
        if (!is_mapped(process->vmemory_table, page_i)) {
            map_page_mem(process->vmemory_table, page_i, (physical_addr_t)pmm_alloc_page(), flags);
        }

        if (page_i > process->brk)
            process->brk = page_i + PAGE_SIZE;
    }
}