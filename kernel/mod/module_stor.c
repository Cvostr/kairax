#include "module_stor.h"
#include "mem/kheap.h"
#include "string.h"
#include "sync/spinlock.h"

#define KERNEL_MODULES_MAX 200
struct module*  kernel_modules[KERNEL_MODULES_MAX];
spinlock_t      kernel_modules_lock = 0;

int mstor_get_free_index_unlock() 
{
    for (int i = 0; i < KERNEL_MODULES_MAX; i ++) {
        if (kernel_modules[i] == NULL) {
            return i;
        }        
    }

    return -1;
}

struct module* mstor_new_module(uint64_t size)
{
    struct module* mod = NULL;
    int idx = 0;

    acquire_spinlock(&kernel_modules_lock);

    idx = mstor_get_free_index_unlock();
    if (idx == -1) 
        goto exit;

    mod = kmalloc(sizeof(struct module));

    // Расширить память в ядре 
    // ВРЕМЕННО!!!
    mod->offset = arch_expand_kernel_mem(size);
    mod->size = size;

    // Сохранить в массиве
    kernel_modules[idx] = mod;

exit:
    release_spinlock(&kernel_modules_lock);
    return mod;
}

int mstor_destroy_module(const char* module_name)
{
    acquire_spinlock(&kernel_modules_lock);

    int rc = -1;
    struct module* mod = NULL;
    int idx = 0;
    for (int i = 0; i < KERNEL_MODULES_MAX; i ++) {
        if (strcmp(kernel_modules[i]->name, module_name) == 0) {
            mod = kernel_modules[i];
            idx = i;
        }
    }

    if (mod == NULL) {
        goto exit;
    } 

    mod->mod_destroy_routine();

    kfree(mod);
    kernel_modules[idx] = NULL;

exit:
    release_spinlock(&kernel_modules_lock);
    return rc;
}