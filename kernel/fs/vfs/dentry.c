#include "dentry.h"
#include "mem/kheap.h"
#include "string.h"
#include "superblock.h"
#include "stdio.h"
#include "inode.h"
#include "vfs.h"

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
    memset(dentry, 0, sizeof(struct dentry));
    kfree(dentry);
}

void dentry_open(struct dentry* dentry)
{
    atomic_inc(&dentry->refs_count);
}

int dentry_close(struct dentry* dentry)
{
    if (atomic_dec_and_test(&dentry->refs_count)) {

        // Переход в состояние неиспользуемое "unused"
    
        if ((dentry->flags & DENTRY_INVALID) == DENTRY_INVALID) {
            
            //printk("destroying dentry %s, inode %i\n", dentry->name, dentry->d_inode->inode);
            // Закрытие inode
            // Внимание! Если живых жестких ссылок не осталось - произойдет удаление
            inode_close(dentry->d_inode);
            // Удаление из dentry родителя
            dentry_remove_subdir(dentry->parent, dentry);
            // Освобождение памяти
            free_dentry(dentry);
        }
    }

    return 0;
}

void dentry_add_subdir(struct dentry* parent, struct dentry* dir)
{
    acquire_spinlock(&parent->lock);
    list_add(parent->subdirs, dir);
    release_spinlock(&parent->lock);
}

void dentry_remove_subdir(struct dentry* parent, struct dentry* dir)
{
    if (parent != NULL) {
        acquire_spinlock(&parent->lock);
        list_remove(parent->subdirs, dir); 
        release_spinlock(&parent->lock);
    }
}

void dentry_reparent(struct dentry* dentr, struct dentry* newparent)
{
    struct dentry* oldParent = dentr->parent;
    
    if (oldParent != newparent)
    {
        dentry_remove_subdir(dentr->parent, dentr);
        dentry_add_subdir(newparent, dentr);
    }
}

int dentry_is_child(struct dentry* d1, struct dentry* d2)
{
    struct dentry* parent = d2->parent;
    while (parent != NULL) {
        if (parent == d1) {
            return 1;
        }

        parent = parent->parent;
    } 

    return 0;
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

        if (slash_pos != NULL) 
        {
            // еще есть разделители /
            strncpy(name_temp, path_temp, slash_pos - path_temp);
            path_temp = slash_pos + 1;
            current = superblock_get_dentry(current->sb, current, name_temp);

            // Убедимся что это директория
            if (current != NULL && (current->flags & DENTRY_TYPE_DIRECTORY) == 0) 
            {
                // TODO: return ENOTDIR
                current = NULL;
                break;
            }

            continue;
        } else 
        {
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

void dentry_debug_tree_entry(struct dentry* den, int level)
{
    for (int i = 0; i < level; i ++)
        printf(" ");

    printk("DEN %s inode %i. refs %i\n", den->name, den->d_inode->inode, den->refs_count);
    if (den->subdirs) {
        struct list_node* current = den->subdirs->head;
        struct dentry* child = NULL;

        for (size_t i = 0; i < den->subdirs->size; i++) {
            
            child = (struct dentry*)current->element;

            dentry_debug_tree_entry(child, level + 1);

            // Переход на следующий элемент
            current = current->next;
        }
    }
}