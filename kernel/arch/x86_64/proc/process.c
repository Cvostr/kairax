#include "process.h"
#include "memory/kernel_vmm.h"
#include "mem/pmm.h"
#include "mem/kheap.h"
#include "string.h"
#include "memory/mem_layout.h"
#include "proc/elf64/elf64.h"
#include "thread.h"
#include "thread_scheduler.h"

int last_pid = 0;

process_t*  create_new_process()
{
    process_t* process = (process_t*)kmalloc(sizeof(process_t));

    // Склонировать таблицу виртуальной памяти ядра
    page_table_t* pml4 = vmm_clone_kernel_memory_map();

    process->name[0] = '\0';
    process->pml4 = pml4;
    process->brk = 0x0;
    process->pid = last_pid++;

    return process;
}

int create_new_process_from_image(char* image)
{
    elf_header_t* elf_header = (elf_header_t*)image;

    if(elf_check_signature(elf_header)) {
        //Это ELF файл
        //printf("bits%i, ARCH-%i, prog_entry_pos-%i \n", elf_header->bits, elf_header->arch, elf_header->prog_entry_pos);

        //Создаем объект процесса
        process_t* proc = create_new_process();

        for (uint32_t i = 0; i < elf_header->section_header_entries_num; i ++) {
            elf_section_header_entry_t* sehentry = elf_get_section_entry(image, i);
            /*printf("SEC: name %s foffset %i size %i type %i flags %i\n", 
                elf_get_string_at(image, sehentry->name_offset),
                sehentry->offset,
                sehentry->size,
                sehentry->type,
                sehentry->flags);*/
        }
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
            memset_vm(proc->pml4, pehentry->v_addr, 0, pehentry->p_memsz);
            // Копировать фрагмент программы в память
            copy_to_vm(proc->pml4, pehentry->v_addr, image + pehentry->p_offset, pehentry->p_filesz);      

            // Создание главного потока и передача выполнения
            thread_t* thr = create_thread(proc, elf_header->prog_entry_pos);
	        add_thread(thr);  
        }
    } else {
        return -1;
    }

    
}

uintptr_t process_brk(process_t* process, void* addr)
{
    uintptr_t uaddr = addr;
    //Выравнивание до размера страницы в большую сторону
    uaddr += (PAGE_SIZE - (uaddr % PAGE_SIZE));
    //До тех пор, пока адрес конца памяти процесса меньше, выделяем страницы
    while((uint64_t)uaddr > process->brk) {
        map_page_mem(process->pml4, process->brk, pmm_alloc_page(), PAGE_USER_ACCESSIBLE | PAGE_WRITABLE | PAGE_PRESENT);
        process->brk += PAGE_SIZE;
    }

    //До тех пор, пока адрес конца памяти процесса больше, освобождаем страницы
    while((uint64_t)uaddr < process->brk) {
        uintptr_t virtual_page_start = process->brk - PAGE_SIZE;
        physical_addr_t phys_addr = get_physical_address(process->pml4, virtual_page_start);
        unmap_page(process->pml4, virtual_page_start);
        pmm_free_page(phys_addr);
        process->brk -= PAGE_SIZE;
    }

    return process->brk;
}

int process_alloc_memory(process_t* process, uintptr_t start, uintptr_t size, uint64_t flags)
{
    uintptr_t start_aligned = start - (start % PAGE_SIZE); // выравнивание в меньшую сторону
    uintptr_t size_aligned = size + (PAGE_SIZE - (size % PAGE_SIZE)); //выравнивание в большую сторону

    for (uintptr_t page_i = start_aligned; page_i < start_aligned + size_aligned; page_i += PAGE_SIZE) {
        map_page_mem(process->pml4, page_i, pmm_alloc_page(), flags);

        if (page_i > process->brk)
            process->brk = page_i + PAGE_SIZE;
    }
}