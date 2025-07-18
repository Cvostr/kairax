#include "keyboard.h"
#include "fs/devfs/devfs.h"
#include "mem/kheap.h"
#include "kairax/string.h"

struct file_operations keyboard_fops;

struct keyboard_buffer* key_buffers[KEY_BUFFERS_MAX];
spinlock_t              key_buffers_lock = 0;
struct keyboard_buffer* main_key_buffer = NULL;

struct keyboard_buffer* new_keyboard_buffer() 
{
    struct keyboard_buffer* result = NULL;
    acquire_spinlock(&key_buffers_lock);

    for (int i = 0; i < KEY_BUFFERS_MAX; i ++) 
    {
        if (key_buffers[i] == NULL) 
        {
            // Создать буфер
            result = kmalloc(sizeof(struct keyboard_buffer));
            memset(result, 0, sizeof(struct keyboard_buffer));

            key_buffers[i] = result;
            goto exit;
        }
    }

exit:
    release_spinlock(&key_buffers_lock);
    return result;
}

void keyboard_buffer_destroy(struct keyboard_buffer* kbuffer)
{
    acquire_spinlock(&key_buffers_lock);

    for (int i = 0; i < KEY_BUFFERS_MAX; i ++) 
    {
        if (key_buffers[i] == kbuffer) 
        {
            // Освободить буфер
            kfree(kbuffer);
            key_buffers[i] = NULL;
            break;
        }
    }

    release_spinlock(&key_buffers_lock);
}

int keyboard_file_open(struct inode *inode, struct file *file)
{
    struct keyboard_buffer* buffer = new_keyboard_buffer();
    file->private_data = buffer;
    // todo : улучшить в file_open
    return 0;
}

ssize_t keyboard_file_read (struct file* file, char* buffer, size_t count, loff_t offset)
{
    struct keyboard_buffer* keybuffer = (struct keyboard_buffer*) file->private_data;
    if (keybuffer->start == keybuffer->end) {
        return 0;
    }

    short pressed = keyboard_get_key_from_buffer(keybuffer);
    buffer[0] = pressed & 0xFF;
    buffer[1] = (pressed >> 8) & 0xFF;

    return 1;
}

int keyboard_file_close(struct inode *inode, struct file *file)
{
    struct keyboard_buffer* keybuffer = (struct keyboard_buffer*) file->private_data;
    keyboard_buffer_destroy(keybuffer);
    return 0;
}

void keyboard_add_event(uint8_t key, uint8_t action)
{
    short event_code = (((short) action) << 8) | key;

    for (int i = 0; i < KEY_BUFFERS_MAX; i ++) 
    {
        if (key_buffers[i] != NULL) 
        {
            struct keyboard_buffer* buffer = key_buffers[i];

            if (buffer->end >= KEY_BUFFER_SIZE) 
                buffer->end = 0;

            buffer->buffer[buffer->end] = event_code;
            buffer->end += 1;
        }
    }
}

short keyboard_get_key_from_buffer(struct keyboard_buffer* buffer)
{
    if (buffer->start != buffer->end) {
        if (buffer->start >= KEY_BUFFER_SIZE) buffer->start = 0;
            buffer->start++;
    } else {
        return 0;
    }

    return buffer->buffer[buffer->start - 1];
}

short keyboard_get_key()
{
    return keyboard_get_key_from_buffer(main_key_buffer);
}

void keyboard_init()
{
    memset(key_buffers, 0, sizeof(key_buffers));
    main_key_buffer = new_keyboard_buffer();

    keyboard_fops.read = keyboard_file_read;
    keyboard_fops.open = keyboard_file_open;
    keyboard_fops.close = keyboard_file_close;
	devfs_add_char_device("keyboard", &keyboard_fops, NULL);
}