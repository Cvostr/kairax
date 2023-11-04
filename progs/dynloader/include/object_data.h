#ifndef _OBJECT_DATA_H
#define _OBJECT_DATA_H

#include "elf.h"

#define OBJECT_NAME_LEN_MAX 50
#define OBJECT_DEPENDENCIES_MAX 30

struct object_data {
    
    uint64_t            base;
    uint64_t            size;

    char                name[OBJECT_NAME_LEN_MAX];
    
    struct object_data* dependencies[OBJECT_DEPENDENCIES_MAX];
    uint32_t            dependencies_count;

    void                *dynamic_section;
    uint64_t            dynamic_sec_size;
    
    void*               dynstr;
    uint64_t            dynstr_size;

    struct elf_symbol*  dynsym;
    uint64_t            dynsym_size;

    struct elf_rela*    rela;
    uint64_t            rela_size;

    // Перемещения для GOT - PLT
    struct elf_rela*    plt_rela;
    uint64_t            plt_rela_size;
};

struct elf_symbol* look_for_symbol(struct object_data* root_obj, const char* name, struct object_data** obj);

#endif