#ifndef KX_MODULE_H
#define KX_MODULE_H

struct kxmodule_header
{
    char mod_name[20];
    int (*mod_init_routine)(void);
    void (*mod_destroy_routine)(void);
};

#define DEFINE_MODULE(name, init_routine, destroy_routine) __attribute__((section(".kxmod_header"))) \
struct kxmodule_header modheader = { \
    .mod_name = name, \
    .mod_init_routine = init_routine, \
    .mod_destroy_routine = destroy_routine, \
};

#endif