#include "arp.h"
#include "kairax/in.h"
#include "string.h"
#include "eth.h"

void arp_handle_packet(struct net_buffer* nbuffer)
{
    struct arp_header* arph = (struct arp_header*) nbuffer->cursor;
    struct nic* nic = nbuffer->netdev;

    if (ntohs(arph->oper) == ARP_OPER_REQ) {
        
        if (ntohs(arph->ptype) == ARP_PTYPE_IPV4) {
            
            if (arph->tpa == nic->ipv4_addr) {

                struct arp_header resp;
                resp.htype = htons(ARP_HTYPE_ETHERNET);
                resp.ptype = htons(ARP_PTYPE_IPV4);
                resp.hlen = 6;  // Длина MAC
                resp.plen = 4;  // Длина IP v4
                resp.oper = htons(ARP_OPER_RESP); // Ответ
                memcpy(resp.sha, nic->mac, 6);
                memcpy(resp.spa, &nic->ipv4_addr, 4);
                memcpy(resp.tha, arph->sha, 6);
                memcpy(resp.tpa_a, arph->spa, 4);

                net_buffer_close(nbuffer);

                eth_send_frame(nic, &resp, sizeof(struct arp_header), resp.tha, ETH_TYPE_ARP);
            }
        }
    }
}