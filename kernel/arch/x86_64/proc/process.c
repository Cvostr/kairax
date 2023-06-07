#include "process.h"
#include "memory/kernel_vmm.h"
#include "mem/pmm.h"
#include "mem/kheap.h"
#include "string.h"
#include "memory/mem_layout.h"
#include "proc/elf64/elf64.h"

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
        printf("bits%i, ARCH-%i, prog_entry_pos-%i \n", elf_header->bits, elf_header->arch, elf_header->prog_entry_pos);

        for (uint32_t i = 0; i < elf_header->section_header_entries_num; i ++) {
            elf_section_header_entry_t* sehentry = elf_get_section_entry(image, i);
            printf("SEC: name %s foffset %i size %i type %i\n", 
                elf_get_string_at(image, sehentry->name_offset),
                sehentry->offset,
                sehentry->size,
                sehentry->type);
        }
    } else {
        return -1;
    }

    process_t* proc = create_new_process();
}

uintptr_t process_brk(process_t* process, void* addr)
{
    uintptr_t uaddr = addr;
    //Выравнивание до размера страницы в большую сторону
    uaddr += (PAGE_SIZE - (uaddr % PAGE_SIZE));
    //До тех пор, пока адрес конца памяти процесса меньше, выделяем страницы
    while((uint64_t)uaddr > process->brk){
        map_page_mem(process->pml4, process->brk, pmm_alloc_page(), PAGE_USER_ACCESSIBLE | PAGE_WRITABLE | PAGE_PRESENT);
        process->brk += PAGE_SIZE;
    }

    return process->brk;
}