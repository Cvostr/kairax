#ifndef _LINKER_H
#define _LINKER_H

void* link(void* arg, int index);

struct elf_symbol* look_for_symbol(struct object_data* root_obj, const char* name, struct object_data** obj);

#endif