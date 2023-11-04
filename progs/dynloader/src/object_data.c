#include "object_data.h"
#include "string.h"
#include "stdbool.h"

struct elf_symbol* look_for_symbol( struct object_data* root_obj,
                                    const char* name,
                                    struct object_data** obj) 
{

    struct elf_symbol* sym_ptr = NULL;
    int i;

    //if (look_in_root == true) {
        for (i = 0; i < root_obj->dynsym_size / sizeof(struct elf_symbol); i ++) {
            sym_ptr = ((struct elf_symbol*) root_obj->dynsym) + i;
            int sym_binding = ELF64_SYM_BIND(sym_ptr->info);
            int sym_type = ELF64_SYM_TYPE(sym_ptr->info);
            char* sym_name = root_obj->dynstr + sym_ptr->name;

            if (strcmp(sym_name, name) == 0 && 
                sym_binding == STB_GLOBAL &&
                sym_ptr->shndx != SHN_UNDEF) 
            {
                // Символ найден
                if (obj) {
                    *obj = root_obj;
                }
                return sym_ptr;
            }
        }
    //}

    //if (look_in_deps == true) {
        // ищем в зависимостях
        for (i = 0; i < root_obj->dependencies_count; i ++) {
            struct object_data* dependency = root_obj->dependencies[i];

            sym_ptr = look_for_symbol(dependency, name, obj);

            if (sym_ptr != NULL) {
                return sym_ptr;
            }
        }
    //}

    return NULL;
}