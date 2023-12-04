#include "module_stor.h"
#include "mem/kheap.h"
#include "string.h"
#include "sync/spinlock.h"
#include "errors.h"

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

struct module* mstor_get_module_with_name(const char* name) 
{
    for (int i = 0; i < KERNEL_MODULES_MAX; i ++) {
        
        if (kernel_modules[i] == NULL) {
            continue;
        }

        if (strcmp(kernel_modules[i]->name, name) == 0) {
            return kernel_modules[i];
        }
    }

    return NULL;
}

struct module* mstor_new_module(uint64_t size, const char* name)
{
    struct module* mod = kmalloc(sizeof(struct module));
    strcpy(mod->name, name);
    mod->size = size;

    return mod;
}

int mstor_register_module(struct module* module)
{
    int idx = 0;
    int rc = -1;

    acquire_spinlock(&kernel_modules_lock);

    idx = mstor_get_free_index_unlock();
    if (idx == -1)
        goto exit;

    // Проверить, не загружен ли уже модуль с таким именем
    if (mstor_get_module_with_name(module->name) != NULL) {
        rc = -ERROR_ALREADY_EXISTS;
        goto exit;
    }

    // Расширить память в ядре 
    // ВРЕМЕННО!!!
    module->offset = arch_expand_kernel_mem(module->size);

    // Сохранить в массиве
    kernel_modules[idx] = module;
    rc = 0;

exit:
    release_spinlock(&kernel_modules_lock);
    return rc;
}

int mstor_destroy_module(const char* module_name)
{
    acquire_spinlock(&kernel_modules_lock);

    int rc = -1;
    struct module* mod = NULL;
    int idx = 0;
    for (int i = 0; i < KERNEL_MODULES_MAX; i ++) {
        
        if (kernel_modules[i] == NULL) {
            continue;
        }

        if (strcmp(kernel_modules[i]->name, module_name) == 0) {
            mod = kernel_modules[i];
            idx = i;
            break;
        }
    }

    if (mod == NULL) {
        goto exit;
    } 

    // Вызов функции закрытия модуля
    if (mod->mod_destroy_routine)
        mod->mod_destroy_routine();

    kfree(mod);
    kernel_modules[idx] = NULL;
    rc = 0;

exit:
    release_spinlock(&kernel_modules_lock);
    return rc;
}