#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <glib.h>

#include "server/clients.h"

GSList *clients;

pthread_mutex_t clients_lock;
pthread_mutexattr_t lock_attr;

void
clients_init(void)
{
    // initialise recursive mutex
    pthread_mutexattr_init(&lock_attr);
    pthread_mutexattr_settype(&lock_attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&clients_lock, &lock_attr);
}

ChatClient*
clients_new(struct sockaddr_in client_addr, int socket)
{
    ChatClient *client = malloc(sizeof(ChatClient));
    client->ip = strdup(inet_ntoa(client_addr.sin_addr));
    client->port = ntohs(client_addr.sin_port);
    client->sock = socket;
    client->nickname = NULL;

    return client;
}

void
clients_add(ChatClient *client)
{
    pthread_mutex_lock(&clients_lock);
    clients = g_slist_append(clients, client);
    pthread_mutex_unlock(&clients_lock);
}

void
clients_print_total(void)
{
    pthread_mutex_lock(&clients_lock);
    printf("Connected clients: %d\n", g_slist_length(clients));
    pthread_mutex_unlock(&clients_lock);
}
