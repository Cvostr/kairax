#include "elf64.h"
#include "string.h"
#include "../process.h"
#include "kstdlib.h"
#include "mem/kheap.h"

int elf_check_signature(struct elf_header* elf_header)
{
    return elf_header->header[0] == 0x7F && !memcmp(&elf_header->header[1], "ELF", 3);
}

struct elf_section_header_entry* elf_get_section_entry(char* image, uint32_t section_index)
{
    struct elf_header* elf_header = (struct elf_header*)image;
    return (struct elf_section_header_entry*)
        (image + elf_header->section_header_table_pos + section_index * elf_header->section_header_entry_size);
}

struct elf_program_header_entry* elf_get_program_entry(char* image, uint32_t program_index)
{
    struct elf_header* elf_header = (struct elf_header*)image;
    return (struct elf_program_header_entry*)
        (image + elf_header->prog_header_table_pos + program_index * elf_header->prog_header_entry_size);

}

char* elf_get_string_at(char* image, uint32_t string_index)
{
    struct elf_header* elf_header = (struct elf_header*)image;
    struct elf_section_header_entry* string_section = elf_get_section_entry(image, elf_header->section_names_index);
    return image + string_section->offset + string_index;
}

int elf_load_process(struct process* process, char* image, uint64_t offset)
{
    struct elf_header* elf_header = (struct elf_header*)image;

    if (!elf_check_signature(elf_header)) {
        return ERROR_BAD_EXEC_FORMAT;
    }

    //Это ELF файл
    for (uint32_t i = 0; i < elf_header->prog_header_entries_num; i ++) {
        struct elf_program_header_entry* pehentry = elf_get_program_entry(image, i);
            
        uint64_t vaddr = pehentry->v_addr + offset;
        size_t aligned_size = align(pehentry->p_memsz, pehentry->alignment);

        /*printf("PE: foffset %i, fsize %i, vaddr %i, memsz %i, align %i, type %i, flags %i\n",
                pehentry->p_offset,
                pehentry->p_filesz,
                pehentry->v_addr,
                pehentry->p_memsz,
                pehentry->alignment,
                pehentry->type,
                pehentry->flags);*/

        if (pehentry->type == ELF_SEGMENT_TYPE_LOAD) {
            uint64_t protection = PAGE_PROTECTION_USER;

            if (pehentry->flags & ELF_SEGMENT_FLAG_EXEC) {
                protection |= PAGE_PROTECTION_EXEC_ENABLE;
            }

            if (pehentry->flags & ELF_SEGMENT_FLAG_WRITE) {
                protection |= PAGE_PROTECTION_WRITE_ENABLE;
            }

            // Выделить память в виртуальной таблице процесса 
            process_alloc_memory(process, vaddr, aligned_size, protection);
            // Заполнить выделенную память нулями
            vm_memset(process->vmemory_table, vaddr, 0, aligned_size);
            // Копировать фрагмент программы в память
            vm_memcpy(process->vmemory_table, vaddr, image + pehentry->p_offset, pehentry->p_filesz);   
        }
    }

    for (uint32_t i = 0; i < elf_header->section_header_entries_num; i ++) {
        // Получить описание секции
        struct elf_section_header_entry* sehentry = elf_get_section_entry(image, i);
        // Получить название секции
        char* section_name = elf_get_string_at(image, sehentry->name_offset);

        /*printf("SEC: name %s foffset %i size %i type %i flags %i\n", 
                elf_get_string_at(image, sehentry->name_offset),
                sehentry->offset,
                sehentry->size,
                sehentry->type,
                sehentry->flags);*/

        if (strcmp(section_name, ".tbss") == 0 && (sehentry->flags & ELF_SECTION_FLAG_TLS)) {
            // Есть секция TLS BSS
            process->tls = kmalloc(sehentry->size);
            memset(process->tls, 0, sehentry->size);
            process->tls_size = sehentry->size;
        } else if (strcmp(section_name, ".tdata") == 0 && (sehentry->flags & ELF_SECTION_FLAG_TLS)) {
            process->tls = kmalloc(sehentry->size);
            memcpy(process->tls, image + sehentry->offset, sehentry->size);
            process->tls_size = sehentry->size;
        } 
    }

    return 0;
}