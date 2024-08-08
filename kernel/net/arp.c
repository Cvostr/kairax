#include "arp.h"
#include "kairax/in.h"
#include "string.h"
#include "eth.h"
#include "list/list.h"
#include "sync/spinlock.h"
#include "mem/kheap.h"

struct arp_v4_cache_entry {
    uint32_t    addr;
    uint8_t     mac[6];
};

struct arp_v4_cache_entry* get_arp_cache_entry(uint32_t addr);

spinlock_t arp_cache_lock = 0;
list_t arp_cache = {0,};

void arp_handle_packet(struct net_buffer* nbuffer)
{
    struct arp_header* arph = (struct arp_header*) nbuffer->cursor;
    struct nic* nic = nbuffer->netdev;

    uint16_t oper = ntohs(arph->oper);

    if (oper == ARP_OPER_REQ) {

        if (ntohs(arph->ptype) == ARP_PTYPE_IPV4) { 

            // Добавить в кэш информацию о том, кто запрашивал
            arp_cache_add(arph->spa, arph->sha);          

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
    } else if (oper == ARP_OPER_RESP) {
        if (ntohs(arph->ptype) == ARP_PTYPE_IPV4) {
            // Добавить в кэш информацию о том, кто ответил
            arp_cache_add(arph->spa, arph->sha);    
        }
    }
}

void arp_cache_add(uint32_t addr, uint8_t* mac)
{
    struct arp_v4_cache_entry* exist = get_arp_cache_entry(addr);
    if (exist != NULL)
        return;

    struct arp_v4_cache_entry* new_entry = kmalloc(sizeof(struct arp_v4_cache_entry));
    new_entry->addr = addr;
    memset(new_entry->mac, mac, 6);

    acquire_spinlock(&arp_cache_lock);
    list_add(&arp_cache, new_entry);
    release_spinlock(&arp_cache_lock);
}

struct arp_v4_cache_entry* get_arp_cache_entry(uint32_t addr)
{
    struct list_node* current_node = arp_cache.head;
    struct arp_v4_cache_entry* entry = NULL;
    struct arp_v4_cache_entry* result = NULL;

    for (size_t i = 0; i < arp_cache.size; i++) {
        entry = (struct pci_device_driver*) current_node->element;
        
        if (entry->addr == addr) {
            result = entry;
            break;
        }

        // Переход на следующий элемент
        current_node = current_node->next;
    }

    return result;
}