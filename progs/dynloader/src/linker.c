#include "../../sdk/sys/syscalls.h"
#include "include/elf.h"
#include "linker.h"
#include "string.h"

extern int cfd;

void* link(void* arg, int index) {
    struct got_plt* got = (struct got_plt*) arg;

    struct object_data* object_data = (struct object_data*) got->unused;

    uint64_t* got_func_addr = ((uint64_t*) arg) + 3 + index; 

    struct elf_rela* relocation = object_data->rela;
    relocation += index;

    /*char ss[5];
    ss[0] = 'w';
    ss[1] = 3;
    ss[2] = '0' + index;
    ss[3] = '0';
    ss[4] = '\n';
    syscall_write(cfd, ss, 5);*/

    struct elf_symbol* sym = object_data->dynsym;
    sym += ELF64_R_SYM(relocation->info);

    char* name = object_data->dynstr + sym->name;

    char name_log[15];
    name_log[0] = 'w';
    name_log[1] = strlen(name);
    strcpy(name_log + 2, name);
    syscall_write(cfd, name_log, 14);
    
    struct object_data* dep = NULL;
    struct elf_symbol* dep_symbol = look_for_symbol(object_data, name, &dep);

    if (dep_symbol == NULL || dep == NULL) {
        return 0;
    }

    name_log[0] = 'w';
    name_log[1] = strlen(dep->name);
    strcpy(name_log + 2, dep->name);
    syscall_write(cfd, name_log, 14);

    return 0;
}

struct elf_symbol* look_for_symbol(struct object_data* root_obj, const char* name, struct object_data** obj) {

    struct elf_symbol* sym_ptr = NULL;

    for (int sym_i = 0; sym_i < root_obj->dynsym_size / sizeof(struct elf_symbol); sym_i ++) {

        struct elf_symbol* sym_ptr = ((struct elf_symbol*) root_obj->dynsym) + sym_i;
        int sym_binding = ELF64_SYM_BIND(sym_ptr->info);
        int sym_type = ELF64_SYM_TYPE(sym_ptr->info);
        char* sym_name = root_obj->dynstr + sym_ptr->name;

        /*char ss[5];
            ss[0] = 'w';
            ss[1] = 3;
            ss[2] = '0' + sym_binding;
            ss[3] = '0' + sym_type;
            ss[4] = '\n';
            syscall_write(cfd, ss, 5);*/

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