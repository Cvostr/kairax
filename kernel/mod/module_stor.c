#include "module_stor.h"
#include "mem/kheap.h"
#include "string.h"
#include "sync/spinlock.h"
#include "errors.h"
#include "list/list.h"

list_t kernel_modules = {0};
spinlock_t      kernel_modules_lock = 0;

struct module* mstor_get_module_with_name(const char* name) 
{
    struct list_node* current_node = kernel_modules.head;
    struct module* mod = NULL;

    for (size_t i = 0; i < kernel_modules.size; i++) {
        
        mod = (struct module*) current_node->element;

        if (strcmp(mod->name, name) == 0) {
            return mod;
        }
        
        // Переход на следующий элемент
        current_node = current_node->next;
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
    int rc = -1;

    acquire_spinlock(&kernel_modules_lock);

    // Проверить, не загружен ли уже модуль с таким именем
    if (mstor_get_module_with_name(module->name) != NULL) {
        rc = -ERROR_ALREADY_EXISTS;
        goto exit;
    }

    // Расширить память в ядре 
    // ВРЕМЕННО!!!
    module->offset = arch_expand_kernel_mem(module->size);

    // Сохранить в массиве
    list_add(&kernel_modules, module);
    rc = 0;

exit:
    release_spinlock(&kernel_modules_lock);
    return rc;
}

int mstor_destroy_module(const char* module_name)
{
    acquire_spinlock(&kernel_modules_lock);

    int rc = -1;
    struct module* mod = mstor_get_module_with_name(module_name);

    if (mod == NULL) {
        goto exit;
    } 

    // Вызов функции закрытия модуля
    if (mod->mod_destroy_routine)
        mod->mod_destroy_routine();

    kfree(mod);
    list_remove(&kernel_modules, mod);
    rc = 0;

exit:
    release_spinlock(&kernel_modules_lock);
    return rc;
}