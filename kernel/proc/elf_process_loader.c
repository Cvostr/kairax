#include "elf_process_loader.h"
#include "string.h"
#include "process.h"
#include "kstdlib.h"
#include "mem/kheap.h"
#include "kairax/errors.h"

int elf_load_process(struct process* process, char* image, uint64_t offset, void** entry_ip, char* interp_path)
{
    struct elf_header* elf_header = (struct elf_header*)image;

    if (!elf_check_signature(elf_header)) {
        return ERROR_BAD_EXEC_FORMAT;
    }

    if (entry_ip) {
        *entry_ip = (void*)elf_header->prog_entry_pos + offset;
    }

    if (interp_path) {
        interp_path[0] = '\0';
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

        if (pehentry->type == ELF_SEGMENT_TYPE_INTERP && interp_path) {
            size_t path_size = pehentry->p_filesz;
            if (path_size > INTERP_PATH_MAX_LEN)
                path_size = INTERP_PATH_MAX_LEN;
            strncpy(interp_path, image + pehentry->p_offset, path_size);
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