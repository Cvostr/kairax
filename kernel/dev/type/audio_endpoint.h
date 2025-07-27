#ifndef AUDIO_ENDPOINT_H
#define AUDIO_ENDPOINT_H

#include "kairax/types.h"
#include "sync/spinlock.h"
#include "fs/vfs/file.h"

struct audio_operations {
    void (*set_volume) ();
    size_t (*on_write) (struct audio_endpoint*, char* buffer, size_t count, size_t offset);
};

struct audio_endpoint_creation_args {
    size_t  sample_buffer_size;
    int     is_input;
    void*   private_data;
    struct audio_operations* ops;
};

// Описывает аудио вход/выход на уровне ядра
struct audio_endpoint {
    unsigned char*  sample_buffer;
    size_t          sample_buffer_size;
    spinlock_t      sample_buffer_lock;
    uint32_t        write_offset;
    uint32_t        read_offset;
    int             is_input;
    void*           private_data;

    int             is_acquired; // Открыт символьный файл или нет

    struct audio_operations ops;
};

struct audio_endpoint* new_audio_endpoint(struct audio_endpoint_creation_args *args);
void free_audio_endpoint(struct audio_endpoint* ep);
void register_audio_endpoint(struct audio_endpoint* ep);

size_t audio_endpoint_gather_samples(struct audio_endpoint* ep, char* out, size_t len);

#endif