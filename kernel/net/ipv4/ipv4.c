#include "ipv4.h"
#include "kairax/in.h"
#include "kairax/string.h"
#include "mem/kheap.h"
#include "net/route.h"

struct ip4_protocol* protocols[20] = {0,};

void ip4_register_protocol(struct ip4_protocol* protocol, int proto)
{
	protocols[proto] = protocol;
}

uint16_t ipv4_calculate_checksum(unsigned char* data, size_t len)
{
    uint32_t sum = 0;
	uint16_t* s = (uint16_t*) kmalloc (len);
	memcpy(s, data, len);
	((struct ip4_packet*) (s))->header_checksum = 0;

	for (int i = 0; i < len / 2; ++i) {
		sum += ntohs(s[i]);
		if (sum > 0xFFFF) {
			sum = (sum >> 16) + (sum & 0xFFFF);
		}
	}

	kfree(s);

	return ~(sum & 0xFFFF) & 0xFFFF;
}

void ip4_handle_packet(struct net_buffer* nbuffer)
{
    struct ip4_packet* ip_packet = (struct ip4_packet*) nbuffer->cursor;
	nbuffer->netw_header = ip_packet;

	int header_size = IP4_IHL(ip_packet->version_ihl) * 4;
	net_buffer_shift(nbuffer, header_size);

	printk("IP4: Version: %i, Header len: %i, Protocol: %i\n", IP4_VERSION(ip_packet->version_ihl), IP4_IHL(ip_packet->version_ihl), ip_packet->protocol);
	uint16_t checksum = ipv4_calculate_checksum(ip_packet, header_size);
    if (checksum != ntohs(ip_packet->header_checksum)) {
        printk("INCORRECT HEADER, rec %i, calc %i\n", ntohs(ip_packet->header_checksum), checksum);
    }

	union ip4uni src;
	src.val = ip_packet->src_ip; 
	printk("IP4 source : %i.%i.%i.%i\n", src.array[0], src.array[1], src.array[2], src.array[3]);
	src.val = ip_packet->dst_ip; 
	printk("IP4 dest : %i.%i.%i.%i\n", src.array[0], src.array[1], src.array[2], src.array[3]);

	struct route4* route = route4_resolve(ip_packet->dst_ip);

	struct ip4_protocol* prot = protocols[ip_packet->protocol];
	if (prot != NULL) {
		prot->handler(nbuffer);
	}
}