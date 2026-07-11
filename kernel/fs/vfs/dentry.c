#include "dentry.h"
#include "mem/kheap.h"
#include "string.h"
#include "superblock.h"
#include "stdio.h"
#include "inode.h"
#include "vfs.h"
#include "kairax/limits.h"

#define SYMLINK_MAX_DEPTH 9
#define SYMLINK_PATH_BUFFER_MAX 512

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

void dentry_purge_recursive(struct dentry* dentry)
{
    // Пройтись по детям
    struct list_node* current = dentry->subdirs->head;
    struct dentry* child = NULL;

    while (current != NULL) {
        
        child = (struct dentry*)current->element;

        // Удалить dentry из списка дочерних
        dentry_remove_subdir(dentry, child);
        // Уничтожение дочернего dentry
        dentry_purge_recursive(child);

        // Переход на следующий элемент
        current = current->next;
    }

    inode_close(dentry->d_inode);

    free_dentry(dentry);
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
        dentr->parent = newparent;
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

int dentry_is_fs_busy(struct dentry* den, int is_root)
{
    // У корневой dentry всегда будет минимум 1 ссылка
    int reqd_refs = is_root ? 1 : 0;
    if (den->refs_count.counter > reqd_refs)
    {
        return TRUE;
    }

    // Проверить детей
    struct list_node* current = den->subdirs->head;
    struct dentry* child = NULL;

    while (current != NULL) {
        
        child = (struct dentry*)current->element;

        if (dentry_is_fs_busy(child, FALSE) == TRUE)
        {
            return TRUE;
        }

        // Переход на следующий элемент
        current = current->next;
    }

    return FALSE;
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

int resolve_next_dentry(struct superblock* sb, struct dentry* parent, const char* name, int flags, int depth, struct dentry **out)
{
    int rc = 0;
    // dentry из суперблока
    struct dentry* result = superblock_get_dentry(sb, parent, name);

    if (result == NULL)
    {
        return -ERROR_NO_FILE;
    }
    else if ((result != NULL) && ((result->d_inode->mode & INODE_TYPE_MASK) == INODE_FLAG_SYMLINK))
    {
        // Это символьная ссылка

        // Если глубина вхождения достаточно большая, то мы скорее всего зациклились
        if (depth == SYMLINK_MAX_DEPTH)
        {
            // Выйдем, чтобы не зациклиться и не переполнить стек
            return -ELOOP;
        }

        if (((flags & O_NOFOLLOW) == O_NOFOLLOW))
        {
            if ((flags & O_PATH) == O_PATH)
            {
                // O_NOFOLLOW | O_PATH - вернем dentry самой ссылки
                *out = result;
                return 0;
            } 
            else 
            {
                return -ELOOP;
            }
        }

        // Выделить память под путь ссылки
        char* symlink_path = kmalloc(SYMLINK_PATH_BUFFER_MAX);
        // Получить путь ссылки
        ssize_t len = inode_readlink(result->d_inode, symlink_path, SYMLINK_PATH_BUFFER_MAX - 1); // -1 для учета терминирующего 0
        symlink_path[len] = 0;
        // Разрезолвить путь символьной ссылки (без NOFOLLOW, PATH)
        rc = vfs_dentry_traverse_path_ex(parent, symlink_path, 0, depth + 1, &result);
        // Освободить память
        kfree(symlink_path);
        // при ошибке - выходим уже сейчас
        if (rc != 0)
            return rc;
    }

    *out = result;
    return 0;
}

int dentry_traverse_path(struct dentry* p_parent, const char* path, int flags, int depth, struct dentry **out)
{
    int rc;
    size_t pathlen, namelen;
    struct dentry* current = p_parent;

    pathlen = strlen(path); 

    // проверка длины всего пути
    if (pathlen == 0)
    {
        *out = p_parent;
        return 0;
    }
    else if (pathlen > PATH_MAX)
    {
        return -ENAMETOOLONG;
    }

    if (p_parent == NULL)
    {
        return -ERROR_NO_FILE;
    }

    char* path_temp = (char*)path;
    char* name_temp = kmalloc(pathlen + 1);

    while (1) 
    {
        // Получить позицию следующего разделителя
        char* slash_pos = strchr(path_temp, '/');

        if (slash_pos != NULL) 
        {
            // еще есть разделители /
            // длина имени файла между двумя /
            namelen = slash_pos - path_temp;

            // проверим посчитанную длину файла
            if (namelen > NAME_MAX)
            {
                rc = -ENAMETOOLONG;
                goto exit_error;
            }
            else if (namelen > 0)
            {
                // искать по дереву имеет смысл если длина файла больше 0
                // сформировать имя файла между /
                strncpy(name_temp, path_temp, namelen);
                // Поискать dentry с именем (флаги не передаются, так как участвуют только при поиске последнего звена)
                rc = resolve_next_dentry(current->sb, current, name_temp, 0, depth, &current);
                if (rc != 0)
                {
                    goto exit_error;
                }

                // Убедимся что это директория
                if (current != NULL && (current->flags & DENTRY_TYPE_DIRECTORY) == 0) 
                {
                    rc = -ERROR_NOT_A_DIRECTORY;
                    goto exit_error;
                }
            }

            // если namelen = 0, то значит что в пути подряд идут несколько ///
            // это допустимо, просто пропускаем их

            // Скипнуть /
            path_temp = slash_pos + 1;

            continue;
        } 
        else 
        {
            // Больше нет разделителей /
            namelen = strlen(path_temp);
            // проверить длину имени файла
            if (namelen > NAME_MAX)
            {
                rc = -ENAMETOOLONG;
                goto exit_error;
            }

            // копировать имя в буфер
            strncpy(name_temp, path_temp, namelen);
            // Разрезолвить последний компонент пути
            rc = resolve_next_dentry(current->sb, current, name_temp, flags, depth, &current);
            if (rc != 0)
            {
                goto exit_error;
            }

            break;
        }
    }

    // при успехе
    rc = 0;
    *out = current;

exit_error:
    kfree(name_temp);
    return rc;
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