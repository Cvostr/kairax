#include <syscalls.h>
#include "include/elf.h"
#include "linker.h"
#include "string.h"
#include "stdio.h"
#include <stddef.h>

extern struct object_data*    root;

void* link(void* arg, int index) {

    struct got_plt* got = (struct got_plt*) arg;
    struct object_data* object_data = (struct object_data*) got->unused;

    // Перемещение в текущем объекте
    struct elf_rela* relocation = (struct elf_rela*)object_data->plt_rela + index;
    int reloc_type = ELF64_R_TYPE(relocation->info);
    int reloc_sym_index = ELF64_R_SYM(relocation->info);

    // Внешний символ в текущем объекте
    struct elf_symbol* sym = (struct elf_symbol*) object_data->dynsym + reloc_sym_index;
    char* name = object_data->dynstr + sym->name;
    
    // Зависимость и символ
    struct object_data* dep = NULL;
    struct elf_symbol* dep_symbol = look_for_symbol(object_data, name, &dep, MODE_LOOK_IN_DEPS | MODE_LOOK_IN_CURRENT);

    if (dep_symbol == NULL || dep == NULL) {
        // Не нашли, ищем в корне
        dep_symbol = look_for_symbol(root, name, &dep, MODE_LOOK_IN_DEPS | MODE_LOOK_IN_CURRENT);
    }

    if (dep_symbol == NULL || dep == NULL) {
        printf("Can't resolve symbol %s\n", name);
        return 0;
    }

    //printf("CL: %s F: %s L: %s \n", object_data->name, name, dep->name);

    // Вычисление адреса функции
    uint64_t func_address = (uint64_t) (dep->base + dep_symbol->value);

    // Сохранение адреса в GOT
    uint64_t* got_rel_addr = (uint64_t*)relocation->offset;
    *got_rel_addr = func_address;

    // Перейти по адресу
    return (void*) func_address;
}