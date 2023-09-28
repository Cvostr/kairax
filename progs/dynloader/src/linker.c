#include "../../sdk/sys/syscalls.h"
#include "include/elf.h"
#include "string.h"

extern int cfd;

void* link(void* arg, int index) {
    struct got_plt* got = (struct got_plt*) arg;

    struct object_data* object_data = (struct object_data*) got->unused;

    uint64_t* got_func_addr = ((uint64_t*) arg) + 3 + index; 

    //if (object_data == 0) {
   //     syscall_write(cfd, "w\3XUI", 5);
    //}

    struct elf_rela* relocation = object_data->rela;
    relocation += index;

    char ss[5];
    ss[0] = 'w';
    ss[1] = 3;
    ss[2] = '0' + index;
    ss[3] = '0';
    ss[4] = '\n';
    syscall_write(cfd, ss, 5);

    struct elf_symbol* sym = object_data->dynsym;
    sym += ELF64_R_SYM(relocation->info);

    char* name = object_data->dynstr + sym->name;

    char name_log[15];
    name_log[0] = 'w';
    name_log[1] = strlen(name);
    strcpy(name_log + 2, name);
    syscall_write(cfd, name_log, 14);

    return 0;
}