#ifndef _NET_BUFFER_H
#define _NET_BUFFER_H

#include "kairax/types.h"
#include "kairax/atomic.h"
#include "dev/type/net_device.h"
#include "kairax/time.h"

#define PRIV_DATA_LEN 20

struct net_buffer {
    
    struct net_buffer* next;

    unsigned char*  buffer;
    size_t          buffer_len;
    struct timeval  time;
    struct nic*     netdev;

    char            priv_data[PRIV_DATA_LEN];

    unsigned char*  cursor;         // Указатель на текущую позицию при считывании
    size_t          cur_len;

    // Заголовки
    unsigned char* link_header;
    unsigned char* netw_header;
    unsigned char* transp_header;
    unsigned char* payload;

    size_t netw_packet_size;        // Размер после заголовка сетевого уровня
    size_t payload_size;            // Размер полезной нагрузки

    size_t payload_read_offset;     // Позиция считывания нагрузки

    atomic_t       refs;
};

struct net_buffer* new_net_buffer(const unsigned char* data, size_t len, struct nic* nic);
struct net_buffer* new_net_buffer_out(size_t len);

size_t net_buffer_get_remain_len(struct net_buffer* nbuffer);
void net_buffer_seek_end(struct net_buffer* nbuffer);
void net_buffer_acquire(struct net_buffer* nbuffer);
void net_buffer_free(struct net_buffer* nbuffer);
void net_buffer_shift(struct net_buffer* nbuffer, int offset);
/// @brief Считывает данные payload в память, сдвигая смещение и уменьшая размер payload
/// @param nbuffer 
/// @param dst 
/// @param count 
/// @return 
size_t net_buffer_read_payload_into(struct net_buffer* nbuffer, char* dst, size_t count);

void net_buffer_add_front(struct net_buffer* nbuffer, const void* data, size_t size);

#endif