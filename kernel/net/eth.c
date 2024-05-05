#include "eth.h"
#include "arp.h"
#include "ipv4.h"
#include "kairax/in.h"
#include "string.h"
#include "mem/kheap.h"

void eth_handle_frame(struct device* dev, unsigned char* data, size_t len)
{
    struct ethernet_frame* frame = (struct ethernet_frame*) data;
    /*
    printk("src: ");
    for (int i = 0; i < 6; i ++) {
        printk("%i ", frame->src[i]);
    }    
    printk("\ndest: ");
    for (int i = 0; i < 6; i ++) {
        printk("%i ", frame->dest[i]);
    }    
    */
    
    uint32_t type = ntohs(frame->type);

    switch (type)
    {
    case ETH_TYPE_ARP:
        arp_handle_packet(dev, frame->payload);
        break;
    case ETH_TYPE_IPV4:
        ip4_handle_packet(frame->payload);
        break;
    default:
        printk("ETH type: %i\n", type);
        break;
    }
}

void eth_send_frame(struct device* dev, unsigned char* data, size_t len, uint8_t* dest, int eth_type)
{
    size_t frame_length = sizeof(struct ethernet_frame) + len;
    struct ethernet_frame* frame = kmalloc(frame_length);
    memcpy(frame->src, dev->nic->mac, 6);
    memcpy(frame->dest, dest, 6);
    frame->type = htons(eth_type);
    memcpy(frame->payload, data, len);
    dev->nic->tx(dev, frame, frame_length);
    kfree(frame);
}