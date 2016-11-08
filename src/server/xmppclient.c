/*
 * xmppclient.c
 *
 * Copyright (C) 2015 James Booth <boothj5@gmail.com>
 *
 * This file is part of Stabber.
 *
 * Stabber is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Stabber is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Stabber.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

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
    if (!client) {
        return;
    }

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
