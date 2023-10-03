#include "../../sdk/sys/syscalls.h"
#include "include/elf.h"
#include "linker.h"
#include "string.h"
#include "stdio.h"

extern int cfd;

void* link(void* arg, int index) {

    struct got_plt* got = (struct got_plt*) arg;
    struct object_data* object_data = (struct object_data*) got->unused;

    // Перемещение в текущем объекте
    struct elf_rela* relocation = (struct elf_rela*)object_data->rela + index;
    int reloc_type = ELF64_R_TYPE(relocation->info);
    int reloc_sym_index = ELF64_R_SYM(relocation->info);

    // Внешний символ в текущем объекте
    struct elf_symbol* sym = (struct elf_symbol*) object_data->dynsym + reloc_sym_index;
    char* name = object_data->dynstr + sym->name;
    
    // Зависимость и символ
    struct object_data* dep = NULL;
    struct elf_symbol* dep_symbol = look_for_symbol(object_data, name, &dep);

    if (dep_symbol == NULL || dep == NULL) {
        printf("LINK FAILED %s\n", name);
        return 0;
    }

    printf("CL: %s F: %s L: %s \n", object_data->name, name, dep->name);

    // Вычисление адреса функции
    void* func_address = (void*) (dep->base + dep_symbol->value);

    // Сохранение адреса в GOT
    uint64_t* got_rel_addr = (uint64_t*)relocation->offset;
    *got_rel_addr = func_address;

    // Перейти по адресу
    return func_address;
}

struct elf_symbol* look_for_symbol(struct object_data* root_obj, const char* name, struct object_data** obj) {

    struct elf_symbol* sym_ptr = NULL;

    for (int sym_i = 0; sym_i < root_obj->dynsym_size / sizeof(struct elf_symbol); sym_i ++) {

        struct elf_symbol* sym_ptr = ((struct elf_symbol*) root_obj->dynsym) + sym_i;
        int sym_binding = ELF64_SYM_BIND(sym_ptr->info);
        int sym_type = ELF64_SYM_TYPE(sym_ptr->info);
        char* sym_name = root_obj->dynstr + sym_ptr->name;

        if (strcmp(sym_name, name) == 0 && 
            sym_binding == STB_GLOBAL &&
            sym_ptr->shndx != SHN_UNDEF) 
        {
            // Символ найден
            *obj = root_obj;
            return sym_ptr;
        }
    }

    // ищем в зависимостях
    for (int dep_i = 0; dep_i < root_obj->dependencies_count; dep_i ++) {
        struct object_data* dependency = root_obj->dependencies[dep_i];

        sym_ptr = look_for_symbol(dependency, name, obj);

        if (sym_ptr != NULL) {
            return sym_ptr;
        }
    }

    return NULL;
}