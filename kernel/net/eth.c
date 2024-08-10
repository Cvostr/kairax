#include "eth.h"
#include "arp.h"
#include "ipv4/ipv4.h"
#include "kairax/in.h"
#include "string.h"
#include "mem/kheap.h"

//#define ETH_IN_LOGGING
//#define ETH_OUT_LOGGING

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

#ifdef ETH_IN_LOGGING
    printk("Eth: type: %i, src: %i %i %i %i %i %i\n", ntohs(frame->type), frame->src[0], frame->src[1], frame->src[2], frame->src[3], frame->src[4], frame->src[5]);
    printk("               dest: %i %i %i %i %i %i\n", frame->dest[0], frame->dest[1], frame->dest[2], frame->dest[3], frame->dest[4], frame->dest[5]);
#endif

    if (memcmp(frame->dest, nbuffer->netdev->mac, MAC_DEFAULT_LEN) != 0 && !mac_is_broadcast(frame->dest)) {
        return;
    }
    
    uint16_t type = ntohs(frame->type);
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

int eth_send_nbuffer(struct net_buffer* nbuffer, uint8_t* dest, int type)
{
    uint8_t* src_mac = nbuffer->netdev->mac;
#ifdef ETH_OUT_LOGGING
    printk("Eth: type: %i, src: %i %i %i %i %i %i\n", ntohs(type), src_mac[0], src_mac[1], src_mac[2], src_mac[3], src_mac[4], src_mac[5]);
    printk("               dest: %i %i %i %i %i %i\n", dest[0], dest[1], dest[2], dest[3], dest[4], dest[5]);
#endif

    struct ethernet_frame frame;
    memcpy(&frame.src, src_mac, MAC_DEFAULT_LEN);
    memcpy(&frame.dest, dest, MAC_DEFAULT_LEN);
    frame.type = htons(type);
    net_buffer_add_front(nbuffer, &frame, sizeof(struct ethernet_frame));

    return nbuffer->netdev->tx(nbuffer->netdev, nbuffer->cursor, nbuffer->cur_len);
}