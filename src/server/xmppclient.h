#ifndef __H_XMPPCLIENT
#define __H_XMPPCLIENT

#include <netinet/in.h>

typedef struct xmpp_client_t {
    char *ip;
    int port;
    int sock;
    char *nickname;
} XMPPClient;

XMPPClient* xmppclient_new(struct sockaddr_in client_addr, int socket);
void xmppclient_end_session(XMPPClient *client);

#endif
