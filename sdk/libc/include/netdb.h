#ifndef _NETDB_H
#define _NETDB_H

#include "stddef.h"
#include <sys/cdefs.h>
#include <sys/types.h>

__BEGIN_DECLS

struct hostent {
    char *h_name;			/* Official name of host.  */
    char **h_aliases;		/* Alias list.  */
    int h_addrtype;		    /* Host address type.  */
    socklen_t h_length;		/* Length of address.  */
    char **h_addr_list;		/* List of addresses from name server.  */
};

#define	h_addr	h_addr_list[0]	/* for backward compatibility.  */

struct hostent *gethostbyname(const char *name);
struct hostent *gethostbyname2(const char *name, int af);

#define EAI_FAMILY -1
#define EAI_SOCKTYPE -2
#define EAI_BADFLAGS -3
#define EAI_NONAME -4
#define EAI_SERVICE -5
#define EAI_ADDRFAMILY -6
#define EAI_NODATA -7
#define EAI_MEMORY -8
#define EAI_FAIL -9
#define EAI_AGAIN -10
#define EAI_SYSTEM -11

struct addrinfo {
    int     ai_flags;
    int     ai_family;
    int     ai_socktype;
    int     ai_protocol;
    size_t  ai_addrlen;
    struct sockaddr *ai_addr;
    char   *ai_canonname;
    struct addrinfo *ai_next;
};

int getaddrinfo(const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res) __THROW;
void freeaddrinfo(struct addrinfo *res) __THROW;
const char* gai_strerror(int error) __THROW;

__END_DECLS

#endif