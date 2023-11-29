#ifndef _ELF64_H
#define _ELF64_H

#include "kairax/elf.h"    

struct process;

#define AT_NULL         0               
#define AT_IGNORE       1
#define AT_EXECFD       2
#define AT_PHDR         3
#define AT_PHENT        4
#define AT_PHNUM        5
#define AT_PAGESZ       6
#define AT_BASE         7
#define AT_FLAGS        8
#define AT_ENTRY        9

struct aux_pair {
    int64_t type;
    union {
        int64_t ival;
        void* pval;
    };
};

#define INTERP_PATH_MAX_LEN 64

int elf_check_signature(struct elf_header* elf_header);

struct elf_section_header_entry* elf_get_section_entry(char* image, uint32_t section_index);

struct elf_program_header_entry* elf_get_program_entry(char* image, uint32_t program_index);

char* elf_get_string_at(char* image, uint32_t string_index);

int elf_load_process(struct process* process, char* image, uint64_t offset, void** entry_ip, char* interp_path);

#endif