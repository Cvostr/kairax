#include "eth.h"
#include "arp.h"
#include "ipv4/ipv4.h"
#include "kairax/in.h"
#include "string.h"
#include "mem/kheap.h"

int mac_is_broadcast(uint8_t* mac) 
{
    for (int i = 0; i < MAC_DEFAULT_LEN; i ++) {
        if (mac[i] != 0xFF) 
            return 0;
    }

    return 1;
}

void eth_handle_frame(struct net_buffer* nbuffer)
{
    struct ethernet_frame* frame = (struct ethernet_frame*) nbuffer->cursor;
    nbuffer->link_header = frame;

    if (memcmp(frame->dest, nbuffer->netdev->mac, MAC_DEFAULT_LEN) != 0 && !mac_is_broadcast(frame->dest)) {
        return;
    }
    
    uint32_t type = ntohs(frame->type);
    net_buffer_shift(nbuffer, sizeof(struct ethernet_frame));

    switch (type)
    {
    case ETH_TYPE_ARP:
        arp_handle_packet(nbuffer);
        break;
    case ETH_TYPE_IPV4:
        ip4_handle_packet(nbuffer);
        break;
    default:
        printk("ETH type: %i\n", type);
        break;
    }
}

void eth_send_frame(struct nic* nic, unsigned char* data, size_t len, uint8_t* dest, int eth_type)
{
    size_t frame_length = sizeof(struct ethernet_frame) + len;
    struct ethernet_frame* frame = kmalloc(frame_length);
    memcpy(frame->src, nic->mac, 6);
    memcpy(frame->dest, dest, 6);
    frame->type = htons(eth_type);
    memcpy(frame->payload, data, len);
    nic->tx(nic, frame, frame_length);
    kfree(frame);
}