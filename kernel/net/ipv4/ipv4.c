#include "ipv4.h"
#include "kairax/in.h"
#include "kairax/string.h"
#include "mem/kheap.h"
#include "net/route.h"
#include "net/eth.h"
#include "net/arp.h"
#include "kairax/errors.h"

//#define IPV4_LOGGING

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

#ifdef IPV4_LOGGING
	printk("IP4: Version: %i, Header len: %i, Protocol: %i\n", IP4_VERSION(ip_packet->version_ihl), IP4_IHL(ip_packet->version_ihl), ip_packet->protocol);
#endif
	uint16_t checksum = ipv4_calculate_checksum(ip_packet, header_size);
    if (checksum != ntohs(ip_packet->header_checksum)) {
        printk("INCORRECT HEADER, rec %i, calc %i\n", ntohs(ip_packet->header_checksum), checksum);
    }

#ifdef IPV4_LOGGING
	union ip4uni src;
	src.val = ip_packet->src_ip; 
	printk("IP4 source : %i.%i.%i.%i\n", src.array[0], src.array[1], src.array[2], src.array[3]);
	src.val = ip_packet->dst_ip; 
	printk("IP4 dest : %i.%i.%i.%i\n", src.array[0], src.array[1], src.array[2], src.array[3]);
#endif

	struct route4* route = route4_resolve(ip_packet->dst_ip);

	struct ip4_protocol* prot = protocols[ip_packet->protocol];
	if (prot != NULL) {
		prot->handler(nbuffer);
	}
}

int ip4_send(struct net_buffer* nbuffer, uint32_t dest, uint32_t src, uint8_t prot)
{
	struct route4* route = route4_resolve(dest);

	if (route == NULL) {
#ifdef IPV4_LOGGING
		printk("NO ROUTE!!!!\n");
#endif
		return -ENETUNREACH;
	}

	uint32_t src1 = route->interface->ipv4_addr;

	size_t len = net_buffer_get_remain_len(nbuffer) + sizeof(struct ip4_packet);

	struct ip4_packet pkt;
	memset(&pkt, 0, sizeof(struct ip4_packet));
	pkt.dst_ip = dest;
	pkt.src_ip = htons(src1);
#ifdef __LITTLE_ENDIAN__
	pkt.version_ihl = (4 << 4) | (sizeof(struct ip4_packet) / 4);
#else
	pkt.version_ihl = ((sizeof(struct ip4_packet) / 4) << 4) | 4;
#endif
	pkt.protocol = prot;
	pkt.size = htons(len);
	pkt.ttl = 64;

	pkt.header_checksum = htons(ipv4_calculate_checksum(&pkt, sizeof(struct ip4_packet)));

	net_buffer_add_front(nbuffer, &pkt, sizeof(struct ip4_packet));
	nbuffer->netdev = route->interface;

	uint8_t* mac = arp_cache_get_ip4(dest);

	if (mac == NULL) {
#ifdef IPV4_LOGGING
		printk("NO ARP!!!\n");
#endif
		return -1;
	}

	return eth_send_nbuffer(nbuffer, mac, ETH_TYPE_IPV4);
}