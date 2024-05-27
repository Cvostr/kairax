#ifndef _MODULE_MANAGER_H
#define _MODULE_MANAGER_H

#include "types.h"

struct module {
    char name[30];
    uint64_t offset;
    uint64_t size;
    int (*mod_init_routine)(void);
    void (*mod_destroy_routine)(void);
};

struct module* mstor_new_module(uint64_t size, const char* name);

void free_module(struct module* module);

int mstor_register_module(struct module* module);

int mstor_destroy_module(const char* module_name);

#endif