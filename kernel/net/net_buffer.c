#include "net_buffer.h"
#include "mem/kheap.h"
#include "string.h"

struct net_buffer* new_net_buffer(unsigned char* data, size_t len, struct nic* nic)
{
    unsigned char* mem = kmalloc(len);
    memcpy(mem, data, len);

    struct net_buffer* nbuffer = kmalloc(sizeof(struct net_buffer));
    memset(nbuffer, 0, sizeof(struct net_buffer));
    nbuffer->buffer = mem;
    nbuffer->buffer_len = len;
    nbuffer->netdev = nic;

    nbuffer->cursor = mem;
    nbuffer->cur_len = len;

    return nbuffer;
}

void net_buffer_shift(struct net_buffer* nbuffer, int offset)
{
    nbuffer->cursor += offset;
    nbuffer->cur_len -= offset;
}

void net_buffer_close(struct net_buffer* nbuffer)
{
    if (atomic_dec_and_test(&nbuffer->refs)) {
        kfree(nbuffer->buffer);
        kfree(nbuffer);
    }
}