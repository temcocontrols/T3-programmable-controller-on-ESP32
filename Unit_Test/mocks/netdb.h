#ifndef MOCK_NETDB_H
#define MOCK_NETDB_H

#include "lwip/netdb.h"

#ifndef EAI_NONAME
#define EAI_NONAME (-2)
#endif
#ifndef EAI_FAIL
#define EAI_FAIL (-4)
#endif
#ifndef EAI_FAMILY
#define EAI_FAMILY (-6)
#endif
#ifndef EAI_MEMORY
#define EAI_MEMORY (-10)
#endif
#ifndef TRY_AGAIN
#define TRY_AGAIN 2
#endif

static inline int getaddrinfo(const char *nodename,
                              const char *servname,
                              const struct addrinfo *hints,
                              struct addrinfo **res)
{
    return lwip_getaddrinfo(nodename, servname, hints, res);
}

static inline void freeaddrinfo(struct addrinfo *ai)
{
    lwip_freeaddrinfo(ai);
}

#endif
