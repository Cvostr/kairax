#ifndef _ARPA_INET_H
#define _ARPA_INET_H

#include <sys/cdefs.h>
#include <sys/types.h>
#include <netinet/in.h>

__BEGIN_DECLS

int inet_aton(const char* cp, struct in_addr* inp);
in_addr_t inet_addr(const char* cp);

char* inet_ntoa(struct in_addr in);
char* inet_ntoa_r(struct in_addr in, char* buf);

const char* inet_ntop (int af, const void* src, char* dst, size_t len);

__END_DECLS

#endif