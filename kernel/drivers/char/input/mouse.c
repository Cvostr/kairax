#include "mouse.h"
#include "fs/devfs/devfs.h"
#include "mem/kheap.h"
#include "kairax/string.h"
#include "kstdlib.h"

struct file_operations mouse_fops;

struct mouse_buffer*    mouse_buffers[KEY_BUFFERS_MAX];
spinlock_t              mouse_buffers_lock = 0;

int mouse_file_open(struct inode *inode, struct file *file);
ssize_t mouse_file_read (struct file* file, char* buffer, size_t count, loff_t offset);
int mouse_file_close(struct inode *inode, struct file *file);

struct mouse_buffer* new_mouse_buffer() 
{
    struct mouse_buffer* result = NULL;
    acquire_spinlock(&mouse_buffers_lock);

    for (int i = 0; i < KEY_BUFFERS_MAX; i ++) 
    {
        if (mouse_buffers[i] == NULL) 
        {
            // Создать буфер
            result = kmalloc(sizeof(struct mouse_buffer));
            memset(result, 0, sizeof(struct mouse_buffer));

            mouse_buffers[i] = result;
            goto exit;
        }
    }

exit:
    release_spinlock(&mouse_buffers_lock);
    return result;
}

void mouse_buffer_destroy(struct mouse_buffer* kbuffer)
{
    acquire_spinlock(&mouse_buffers_lock);

    for (int i = 0; i < KEY_BUFFERS_MAX; i ++) 
    {
        if (mouse_buffers[i] == kbuffer) 
        {
            // Освободить буфер
            kfree(kbuffer);
            mouse_buffers[i] = NULL;
            break;
        }
    }

    release_spinlock(&mouse_buffers_lock);
}

void mouse_add_event(struct mouse_event* event)
{
    for (int i = 0; i < KEY_BUFFERS_MAX; i ++) 
    {
        if (mouse_buffers[i] != NULL) 
        {
            struct mouse_buffer* buffer = mouse_buffers[i];

            if (buffer->end >= KEY_BUFFER_SIZE) 
                buffer->end = 0;

            memcpy(&buffer->events[buffer->end], event, sizeof(struct mouse_event));
            buffer->end += 1;
        }
    }
}

void mouse_init()
{
    memset(mouse_buffers, 0, sizeof(mouse_buffers));

    mouse_fops.read = mouse_file_read;
    mouse_fops.open = mouse_file_open;
    mouse_fops.close = mouse_file_close;
	devfs_add_char_device("mouse", &mouse_fops, NULL);
}

int mouse_file_open(struct inode *inode, struct file *file)
{
    struct mouse_buffer* buffer = new_mouse_buffer();
    // Если все слоты закончились - надо вернуть ошибку
    if (buffer == NULL)
    {
        return -EEXIST;
    }

    file->private_data = buffer;
    return 0;
}

ssize_t mouse_file_read (struct file* file, char* buffer, size_t count, loff_t offset)
{
    struct mouse_buffer* keybuffer = (struct mouse_buffer*) file->private_data;
    
    if (keybuffer->start == keybuffer->end) 
    {
        return 0;
    }
    else 
    {
        if (keybuffer->start >= KEY_BUFFER_SIZE) 
        {
            keybuffer->start = 0;
        }
        
        keybuffer->start++;
    }

    struct mouse_event* event = &keybuffer->events[keybuffer->start - 1];
    size_t size_to_copy = MIN(sizeof(struct mouse_event), count);
    memcpy(buffer, event, size_to_copy);

    return size_to_copy;
}

int mouse_file_close(struct inode *inode, struct file *file)
{
    struct mouse_buffer* keybuffer = (struct mouse_buffer*) file->private_data;
    mouse_buffer_destroy(keybuffer);
    return 0;
}
