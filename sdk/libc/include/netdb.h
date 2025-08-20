#ifndef _NETDB_H
#define _NETDB_H

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

__END_DECLS

#endif