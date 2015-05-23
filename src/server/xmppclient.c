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
    client->username = NULL;
    client->password = NULL;
    client->resource = NULL;

    return client;
}

void
xmppclient_end_session(XMPPClient *client)
{
    if (client) {
        if (client->sock) {
            shutdown(client->sock, 2);
            while (recv(client->sock, NULL, 1, 0) > 0) {}
            close(client->sock);
        }
        free(client->ip);
        free(client->username);
        free(client->password);
        free(client->resource);
        free(client);
    }
}
