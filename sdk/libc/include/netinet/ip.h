#ifndef _NETINET_IP_H
#define _NETINET_IP_H

#include <inttypes.h>
#include <netinet/in.h>

__BEGIN_DECLS

struct ip {
#if __LITTLE_ENDIAN__
  unsigned int ip_hl:4;		/* header length */
  unsigned int ip_v:4;		/* version */
#endif
#if __BIG_ENDIAN__
  unsigned int ip_v:4;		/* version */
  unsigned int ip_hl:4;		/* header length */
#endif
  unsigned char ip_tos;		/* type of service */
  unsigned short ip_len;		/* total length */
  unsigned short ip_id;		/* identification */
  unsigned short ip_off;		/* fragment offset field */
  unsigned char ip_ttl;		/* time to live */
  unsigned char ip_p;			/* protocol */
  unsigned short ip_sum;		/* checksum */
  struct in_addr ip_src, ip_dst;	/* source and dest address */
};

__END_DECLS

#endif