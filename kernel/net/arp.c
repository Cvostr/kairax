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

                // Формируем структуру ответа 
                struct arp_header resp;
                resp.htype = htons(ARP_HTYPE_ETHERNET);
                resp.ptype = htons(ARP_PTYPE_IPV4);
                resp.hlen = 6;  // Длина MAC
                resp.plen = 4;  // Длина IP v4
                resp.oper = htons(ARP_OPER_RESP); // Ответ
                memcpy(resp.sha, nic->mac, 6);
                memcpy(resp.spa_a, &nic->ipv4_addr, 4);
                memcpy(resp.tha, arph->sha, 6);
                memcpy(resp.tpa_a, arph->spa_a, 4);

                // Создаем netbuffer и добавляем к нему структуру ответа
                struct net_buffer* response_nbuffer = new_net_buffer_out(512);
                net_buffer_add_front(response_nbuffer, &resp, sizeof(struct arp_header));
                response_nbuffer->netdev = nic;

                // Послать ответ
                eth_send_nbuffer(response_nbuffer, resp.tha, ETH_TYPE_ARP);

                // Освободить буфер ответа
                net_buffer_free(response_nbuffer);
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
    memcpy(new_entry->mac, mac, 6);

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

uint8_t* arp_cache_get_ip4(uint32_t addr)
{
    struct arp_v4_cache_entry* entry = get_arp_cache_entry(addr);

    if (entry == NULL) { // TODO: сделать запрос!!
        return NULL;
    }

    return entry->mac;
}