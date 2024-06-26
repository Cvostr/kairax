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

    unsigned char*  cursor;
    size_t          cur_len;

    // Заголовки
    unsigned char* link_header;
    unsigned char* netw_header;
    unsigned char* transp_header;
    unsigned char* payload;

    atomic_t       refs;
};

struct net_buffer* new_net_buffer(unsigned char* data, size_t len, struct nic* nic);

void net_buffer_close(struct net_buffer* nbuffer);

void net_buffer_shift(struct net_buffer* nbuffer, int offset);

#endif