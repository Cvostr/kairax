#include "arp.h"
#include "kairax/in.h"
#include "string.h"
#include "eth.h"

void arp_handle_packet(struct device* dev, unsigned char* data)
{
    struct arp_header* arph = (struct arp_header*) data;

    if (ntohs(arph->oper) == ARP_OPER_REQ) {
        
        if (ntohs(arph->ptype) == ARP_PTYPE_IPV4) {
            
            if (arph->tpa == dev->nic->ipv4_addr) {
                /*printk("ARP req, TPA :\n");
                for (int i = 0; i < 4; i ++) {
                    printk("%i ", arph->tpa_a[i]);
                }*/
        
                struct arp_header resp;
                resp.htype = htons(ARP_HTYPE_ETHERNET);
                resp.ptype = htons(ARP_PTYPE_IPV4);
                resp.hlen = 6;  // Длина MAC
                resp.plen = 4;  // Длина IP v4
                resp.oper = htons(ARP_OPER_RESP); // Ответ
                memcpy(resp.sha, dev->nic->mac, 6);
                memcpy(resp.spa, &dev->nic->ipv4_addr, 4);
                memcpy(resp.tha, arph->sha, 6);
                memcpy(resp.tpa_a, arph->spa, 4);

                eth_send_frame(dev, &resp, sizeof(struct arp_header), resp.tha, ETH_TYPE_ARP);
            }
        }
    }
}