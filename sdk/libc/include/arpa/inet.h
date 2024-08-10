#ifndef _ARPA_INET_H
#define _ARPA_INET_H

#include <sys/cdefs.h>
#include <sys/types.h>
#include <netinet/in.h>

__BEGIN_DECLS

int inet_aton(const char* cp, struct in_addr* inp);
in_addr_t inet_addr(const char* cp);

__END_DECLS

#endif