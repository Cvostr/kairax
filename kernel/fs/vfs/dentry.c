#include "dentry.h"
#include "mem/kheap.h"
#include "string.h"

struct dentry* new_dentry()
{
    struct dentry* result = kmalloc(sizeof(struct dentry));
    memset(result, 0, sizeof(struct dentry));
    result->subdirs = create_list();
    return result;
}

void free_dentry(struct dentry* dentry)
{
    free_list(dentry->subdirs);
    kfree(dentry);
}

void dentry_open(struct dentry* dentry)
{
    if (dentry->parent) {
        dentry_open(dentry->parent);
    }

    atomic_inc(&dentry->refs_count);
}

void dentry_close(struct dentry* dentry)
{
    if (atomic_dec_and_test(&dentry->refs_count)) {
        
        if (dentry->parent) {
            // Если есть родитель, удалиться его списка и уменьшить счетчик
            dentry_remove_subdir(dentry->parent, dentry);
            dentry_close(dentry->parent);
        }

        // Закрыть свои дочерние dentries
        struct list_node* current = dentry->subdirs->head;
        for (unsigned int i = 0; i < dentry->subdirs->size; i++) {
            struct dentry* child = (struct dentry*)current->element;
            dentry_close(child);
            
            current = current->next;    
        }

        free_dentry(dentry);
    }
}

void dentry_add_subdir(struct dentry* parent, struct dentry* dir)
{
    acquire_spinlock(&parent->lock);
    list_add(parent->subdirs, dir); 
    release_spinlock(&parent->lock);
}

void dentry_remove_subdir(struct dentry* parent, struct dentry* dir)
{
    acquire_spinlock(&parent->lock);
    list_remove(parent->subdirs, dir); 
    release_spinlock(&parent->lock);
}

struct dentry* dentry_get_child_with_name(struct dentry* parent, const char* child_name)
{
    struct dentry* result = NULL;

    acquire_spinlock(&parent->lock);
    
    if(parent == NULL) {
        goto exit;
    }

    struct list_node* current = parent->subdirs->head;
    struct dentry* child = (struct dentry*)current->element;

    for (unsigned int i = 0; i < parent->subdirs->size; i++) {
        
        if (strcmp(child->name, child_name) == 0) {
            result = child;
            goto exit;
        }

        current = current->next;
        child = (struct dentry*)current->element;
    }

exit:
    release_spinlock(&parent->lock);

    return result;
}