#ifndef _IP_ICMP_H
#define _IP_ICMP_H

#include <inttypes.h>

#define ICMP_ECHOREPLY      0
#define ICMP_DEST_UNREACH   3
#define ICMP_SOURCE_QUENCH	4
#define ICMP_REDIRECT		5
#define ICMP_ECHO		    8
#define ICMP_TIME_EXCEEDED	11
#define ICMP_PARAMETERPROB	12
#define ICMP_TIMESTAMP		13
#define ICMP_TIMESTAMPREPLY	14
#define ICMP_INFO_REQUEST	15
#define ICMP_INFO_REPLY		16
#define ICMP_ADDRESS		17
#define ICMP_ADDRESSREPLY	18

struct icmphdr {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;

    union {

        struct {
            uint16_t id;
            uint16_t sequence;
        } echo;

        uint32_t gateway;

        struct {
            uint16_t unused;
            uint16_t mtu;
        } frag;

    } un;
};

#endif