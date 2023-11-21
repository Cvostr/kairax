#include "dentry.h"
#include "mem/kheap.h"
#include "string.h"
#include "superblock.h"
#include "stdio.h"

struct dentry* new_dentry()
{
    struct dentry* result = kmalloc(sizeof(struct dentry));
    memset(result, 0, sizeof(struct dentry));
    result->subdirs = create_list();
    return result;
}

void free_dentry(struct dentry* dentry)
{
    //printf("Freeing dentry, %s\n",dentry->name);
    free_list(dentry->subdirs);
    kfree(dentry);
}

void dentry_open(struct dentry* dentry)
{
    atomic_inc(&dentry->refs_count);
}

void dentry_close(struct dentry* dentry)
{
    //printf(" %i ", dentry->refs_count.counter);
    //printf_stdout("CLOSING  %s LEft %i\n", dentry->name, dentry->refs_count.counter);
    if (atomic_dec_and_test(&dentry->refs_count)) {
        
        if (dentry->parent) {
            // Если есть родитель, удалиться из его списка и уменьшить счетчик родителя
            dentry_remove_subdir(dentry->parent, dentry);
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

    atomic_inc(&parent->refs_count);
    atomic_inc(&dir->refs_count); 
}

void dentry_remove_subdir(struct dentry* parent, struct dentry* dir)
{
    acquire_spinlock(&parent->lock);
    list_remove(parent->subdirs, dir); 
    release_spinlock(&parent->lock);

    dentry_close(parent);
    dentry_close(dir);
}

void dentry_reparent(struct dentry* dentr, struct dentry* newparent)
{
    dentry_remove_subdir(dentr->parent, dentr);
    dentry_add_subdir(newparent, dentr);
}

struct dentry* dentry_get_child_with_name(struct dentry* parent, const char* child_name)
{
    struct dentry* result = NULL;

    acquire_spinlock(&parent->lock);
    
    if(parent == NULL) {
        goto exit;
    }

    if (strcmp(child_name, ".") == 0) {
        result = parent;
        goto exit;
    }

    if (strcmp(child_name, "..") == 0) {

        if (parent->parent)
            result = parent->parent;
        else 
            result = parent;

        goto exit;
    }

    struct list_node* current = parent->subdirs->head;
    struct dentry* child = NULL;

    for (size_t i = 0; i < parent->subdirs->size; i++) {
        
        child = (struct dentry*)current->element;

        if (strcmp(child->name, child_name) == 0) {
            result = child;
            goto exit;
        }

        // Переход на следующий элемент
        current = current->next;
    }

exit:
    release_spinlock(&parent->lock);

    return result;
}

struct dentry* dentry_traverse_path(struct dentry* p_parent, const char* path)
{
    struct dentry* current = p_parent;

    if (strlen(path) == 0)
        return p_parent;

    char* path_temp = (char*)path;
    char* name_temp = kmalloc(strlen(path) + 1);

    while (1) {
        
        if (current == NULL) {
            break;
        }

        // Получить позицию следующего разделителя
        char* slash_pos = strchr(path_temp, '/');

        if (slash_pos != NULL) {
            // еще есть разделители /
            strncpy(name_temp, path_temp, slash_pos - path_temp);
            path_temp = slash_pos + 1;
            current = superblock_get_dentry(current->sb, current, name_temp);
            continue;
        } else {
            // Больше нет разделителей /
            strncpy(name_temp, path_temp, strlen(path_temp));
            current = superblock_get_dentry(current->sb, current, name_temp);
            break;
        }
    }

    kfree(name_temp);

    return current;
}

void dentry_get_absolute_path(struct dentry* p_dentry, size_t* p_required_size, char* p_result)
{
    if (p_dentry->parent) {
        dentry_get_absolute_path(p_dentry->parent, p_required_size, p_result);
    }

    if (strlen(p_dentry->name) > 0) {

        if (p_required_size) {
            *p_required_size += (strlen(p_dentry->name) + 1);
        }

        if (p_result) {
            strcat(p_result, "/");
            strcat(p_result, p_dentry->name);
        }
    }
}