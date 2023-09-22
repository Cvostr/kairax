#include "../../sdk/sys/syscalls.h"
#include "include/elf.h"
#include "string.h"

extern int cfd;

void* link(void* arg, int index) {
    struct got_plt* got = (struct got_plt*) arg;
    struct object_data* main_object_data = (struct object_data*) got->unused;
    uint64_t* got_func_addr = (uint64_t*) (arg + 3 + index); 

    syscall_write(cfd, "w\5ALIVE", 7);

    char ss[4];
    ss[0] = 'w';
    ss[1] = 2;
    ss[2] = '0' + index;
    ss[3] = '\n';
    syscall_write(cfd, ss, 4);

    struct elf_symbol* sym = main_object_data->dynsym;
    sym += index;

    char* name = main_object_data->dynstr += sym->name;

    syscall_write(cfd, "w\7LINKER ", 7);

    char name_log[15];
    name_log[0] = 'w';
    name_log[1] = strlen(name);
    strcpy(name_log + 2, name);
    syscall_write(cfd, name_log, 14);

    return 0;
}