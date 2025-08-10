#include "net_buffer.h"
#include "mem/kheap.h"
#include "string.h"
#include "kairax/kstdlib.h"

struct net_buffer* new_net_buffer(const unsigned char* data, size_t len, struct nic* nic)
{
    unsigned char* mem = kmalloc(len);
    if (mem == NULL) {
        return NULL;
    }

    memcpy(mem, data, len);

    struct net_buffer* nbuffer = kmalloc(sizeof(struct net_buffer));
    if (nbuffer == NULL) {
        kfree(mem);
        return NULL;
    }
    
    memset(nbuffer, 0, sizeof(struct net_buffer));
    nbuffer->buffer = mem;
    nbuffer->buffer_len = len;
    nbuffer->netdev = nic;

    nbuffer->cursor = mem;
    nbuffer->cur_len = len;

    return nbuffer;
}

struct net_buffer* new_net_buffer_out(size_t len)
{
    unsigned char* mem = kmalloc(len);
    if (mem == NULL) {
        return NULL;
    }

    memset(mem, 0, len);

    struct net_buffer* nbuffer = kmalloc(sizeof(struct net_buffer));
    if (nbuffer == NULL) {
        kfree(mem);
        return NULL;
    }

    memset(nbuffer, 0, sizeof(struct net_buffer));
    nbuffer->buffer = mem;
    nbuffer->buffer_len = len;

    // Курсор в самом конце
    nbuffer->cursor = mem + nbuffer->buffer_len;
    nbuffer->cur_len = 0;

    return nbuffer;
}

size_t net_buffer_get_remain_len(struct net_buffer* nbuffer)
{
    return nbuffer->cur_len;
}

size_t net_buffer_get_payload_remain_len(struct net_buffer* nbuffer)
{
    return nbuffer->payload_size - nbuffer->payload_read_offset;
}

void net_buffer_seek_end(struct net_buffer* nbuffer)
{
    nbuffer->cursor += nbuffer->buffer_len;
    nbuffer->cur_len = 0;
}

void net_buffer_shift(struct net_buffer* nbuffer, int offset)
{
    nbuffer->cursor += offset;
    nbuffer->cur_len -= offset;
}

size_t net_buffer_read_payload_into(struct net_buffer* nbuffer, char* dst, size_t count)
{
    size_t payload_remaining = nbuffer->payload_size - nbuffer->payload_read_offset;
    size_t readable = MIN(count, payload_remaining);
    unsigned char* current_payload = nbuffer->payload + nbuffer->payload_read_offset;

    memcpy(dst, current_payload, readable);

    nbuffer->payload_read_offset += readable;
    net_buffer_shift(nbuffer, readable);

    return readable;
}

void net_buffer_add_front(struct net_buffer* nbuffer, const void* data, size_t size)
{
    nbuffer->cursor -= size;
    nbuffer->cur_len += size;
    memcpy(nbuffer->cursor, data, size);
}

void net_buffer_acquire(struct net_buffer* nbuffer)
{
    atomic_inc(&nbuffer->refs);
}

void net_buffer_free(struct net_buffer* nbuffer)
{
    if (atomic_dec_and_test(&nbuffer->refs)) {
        kfree(nbuffer->buffer);
        kfree(nbuffer);
    }
}