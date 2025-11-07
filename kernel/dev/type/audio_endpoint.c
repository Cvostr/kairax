#include "audio_endpoint.h"
#include "mem/kheap.h"
#include "kairax/string.h"
#include "fs/devfs/devfs.h"

int audio_endpoint_file_open(struct inode *inode, struct file *file);
int audio_endpoint_file_close(struct inode *inode, struct file *file);
int audio_endpoint_ioctl(struct file* file, uint64_t request, uint64_t arg);
ssize_t audio_endpoint_fwrite(struct file *file, char* buffer, size_t count, size_t offset);

struct file_operations audio_output_widget_fops = {
    .open = audio_endpoint_file_open,
    .close = audio_endpoint_file_close,
    .ioctl = audio_endpoint_ioctl,
    .write = audio_endpoint_fwrite
};
struct file_operations audio_input_widget_fops = {
    .open = audio_endpoint_file_open,
    .close = audio_endpoint_file_close,
    .ioctl = audio_endpoint_ioctl
};
int registered_audio_eps = 0;

struct audio_endpoint* new_audio_endpoint(struct audio_endpoint_creation_args *args)
{
    struct audio_endpoint* audio = kmalloc(sizeof(struct audio_endpoint));
    if (audio == NULL)
    {
        return NULL;
    }

    memset(audio, 0, sizeof(struct audio_endpoint));

    // Заполнить данными из args
    audio->private_data = args->private_data;
    audio->is_input = args->is_input;
    memcpy(&audio->ops, args->ops, sizeof(struct audio_operations));
    audio->sample_buffer_size = args->sample_buffer_size;
    // Выделяем буфер под сэмплы
    audio->sample_buffer = kmalloc(audio->sample_buffer_size);
    if (audio->sample_buffer == NULL)
    {
        kfree(audio);
        return NULL;
    }
    memset(audio->sample_buffer, 0, audio->sample_buffer_size);

    return audio;
}

void free_audio_endpoint(struct audio_endpoint* ep)
{
    if (ep->sample_buffer != NULL)
    {
        kfree(ep->sample_buffer);
    }

    kfree(ep);
}

void register_audio_endpoint(struct audio_endpoint* ep)
{
    char name[6];
    strcpy(name, "aud");
    // TODO: Переделать это говно!!!
    name[3] = (registered_audio_eps++) + 0x30;
    name[4] = 0;

    struct file_operations* ops = ep->is_input ? &audio_input_widget_fops : &audio_output_widget_fops;

    devfs_add_char_device(name, ops, ep);
}

size_t audio_endpoint_gather_samples(struct audio_endpoint* ep, char* out, size_t len)
{
    if (ep->read_offset == ep->write_offset)
    {
        // Нечего читать - выходим
        return 0;
    }

    size_t i;
    size_t sample_buffer_sz = ep->sample_buffer_size;
    uint64_t* sample_buffer32 = ep->sample_buffer;

    for (i = 0; i < len / sizeof(uint64_t); i ++) 
    {
        if (ep->read_offset == ep->write_offset)
            break;
            
        ((uint64_t*)out)[i] = sample_buffer32[(ep->read_offset % sample_buffer_sz) / sizeof(uint64_t)];
        ep->read_offset += sizeof(uint64_t);
    }

    return i * sizeof(uint64_t);
}

int audio_endpoint_file_open(struct inode *inode, struct file *file)
{
    struct audio_endpoint* ep = inode->private_data;

    if (ep->is_acquired == TRUE)
        return -ENOTEMPTY;

    ep->is_acquired = TRUE;

    return 0;
}

int audio_endpoint_file_close(struct inode *inode, struct file *file)
{
    struct audio_endpoint* ep = inode->private_data;
    // Файл снова можно открыть
    ep->is_acquired = FALSE;
    // Сбросить счетчики
    ep->read_offset = 0;
    ep->write_offset = 0;
    return 0;
}

ssize_t audio_endpoint_fwrite(struct file *file, char* buffer, size_t count, size_t offset)
{
    struct audio_endpoint* ep = file->inode->private_data;

    if (ep->ops.on_write != NULL)
    {
        return ep->ops.on_write(ep, buffer, count, offset);
    }

    size_t sample_buffer_sz = ep->sample_buffer_size;

    for (size_t i = 0; i < count; i ++) 
    {    
        while (ep->write_offset == ep->read_offset + sample_buffer_sz)
        {
            // Ожидаем, пока драйвер устройства все считает
        }

        // записываем байт
        ep->sample_buffer[ep->write_offset++ % sample_buffer_sz] = buffer[i];
    }

    return count;
}

int audio_endpoint_ioctl(struct file* file, uint64_t request, uint64_t arg)
{
    struct audio_endpoint* ep = file->inode->private_data;
    struct audio_operations *ops = &ep->ops;

    switch (request) {
        case AUDCTL_SET_VOLUME:
            uint16_t volume = arg & 0xFFFF;
            uint8_t volume_left = arg & 0xFF;
            uint8_t volume_right = (arg >> 8);
            if (volume_left > 100 || volume_right > 100)
                return -EINVAL;
            if (ops->set_volume)
                ops->set_volume(ep, volume_left, volume_right);
            break;
        default:
            return -EINVAL;
    }

    return 0;
}