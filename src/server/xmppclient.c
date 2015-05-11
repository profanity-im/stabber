#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <glib.h>

#include "server/xmppclient.h"

XMPPClient*
xmppclient_new(struct sockaddr_in client_addr, int socket)
{
    XMPPClient *client = malloc(sizeof(XMPPClient));
    client->ip = strdup(inet_ntoa(client_addr.sin_addr));
    client->port = ntohs(client_addr.sin_port);
    client->sock = socket;
    client->nickname = NULL;

    return client;
}

void
xmppclient_end_session(XMPPClient *client)
{
    close(client->sock);

    free(client->ip);
    free(client->nickname);
    free(client);
}
