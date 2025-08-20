#ifndef _ARPA_NAMESER_H
#define _ARPA_NAMESER_H

#include <stddef.h>
#include <inttypes.h>
#include <sys/cdefs.h>

__BEGIN_DECLS

#define OP_QUERY    0
#define OP_IQUERY   1
#define OP_STATUS   2
#define OP_NOTIFY   4
#define OP_UPDATE   5

#define RCODE_NOERROR   0
#define RCODE_FORMERR   1
#define RCODE_SERVFAIL  2
#define RCODE_NXDOMAIN  3
#define RCODE_NOTIMPL   4
#define RCODE_REFUSED   5

#define CLASS_IN    1

#define TYPE_A      1   /* Host address. */
#define TYPE_NS     2   /* Authoritative server. */
#define TYPE_MD     3   /* Mail destination. */
#define TYPE_MF     4
#define TYPE_CNAME  5   /* Canonical name. */
#define TYPE_SOA    6   /* Start of authority zone. */
#define TYPE_MB     7   /* Mailbox domain name. */
#define TYPE_MG     8   /* Mail group member. */
#define TYPE_MR     9   /* Mail rename name. */
#define TYPE_NULL   10
#define TYPE_WKS    11  /* Well known service. */
#define TYPE_PTR    12  /* Domain name pointer. */
#define TYPE_HINFO  13
#define TYPE_MINFO  14
#define TYPE_MX     15
#define TYPE_TXT    16
#define TYPE_RP     17
#define TYPE_AFSDB  18
#define TYPE_X25    19  /* X_25 calling address. */
#define TYPE_ISDN   20  /* ISDN calling address. */
#define TYPE_RT     21  /* Router. */
#define TYPE_NSAP   22  /* NSAP address. */
#define TYPE_SIG    24  /* Security signature. */
#define TYPE_KEY    25  /* Security key. */
#define TYPE_PX     26  /* X.400 mail mapping. */
#define TYPE_GPOS   27  /* Geographical position (withdrawn). */
#define TYPE_AAAA   28  /* Ip6 Address. */

typedef struct 
{
    uint16_t id;

#ifdef __BIG_ENDIAN__
    unsigned	qr: 1;		/* response flag */
	unsigned	opcode: 4;	/* purpose of message */
	unsigned	aa: 1;		/* authoritative answer */
	unsigned	tc: 1;		/* truncated message */
	unsigned	rd: 1;		/* recursion desired */
	unsigned	ra: 1;		/* recursion available */
	unsigned	unused :1;	/* unused bits (MBZ as of 4.9.3a3) */
	unsigned	ad: 1;		/* authentic data from named */
	unsigned	cd: 1;		/* checking disabled by resolver */
	unsigned	rcode :4;	/* response code */
#else
    unsigned        rd :1;          /* recursion desired */
    unsigned        tc :1;          /* truncated message */
    unsigned        aa :1;          /* authoritive answer */
    unsigned        opcode :4;      /* purpose of message */
    unsigned        qr :1;          /* response flag */
    unsigned        rcode :4;       /* response code */
    unsigned        cd: 1;          /* checking disabled by resolver */
    unsigned        ad: 1;          /* authentic data from named */
    unsigned        unused :1;      /* unused bits (MBZ as of 4.9.3a3) */
    unsigned        ra :1;          /* recursion available */
#endif

    uint16_t        qdcount;    /* number of question entries */
    uint16_t        ancount;    /* number of answer entries */
    uint16_t        nscount;    /* number of authority entries */
    uint16_t        arcount;    /* number of resource entries */

} DNS_HEADER;

typedef struct __attribute__((packed))
{
    uint16_t name;
    uint16_t type;
    uint16_t class;
    uint32_t ttl;
    uint16_t rdlength;
    char     rddata[];
} DNS_ANSWER;

size_t ns_encode_name(char* dst, const char* name);
// Возвращает размер строки
size_t ns_read_name(char* in, char* name);

size_t ns_form_question(char* out, const char* name, int type, int class);
// Возвращает полный размер записи
size_t ns_read_query(char* in, char* name, uint16_t* qtype, uint16_t* qclass);

__END_DECLS

#endif