#ifndef __H_CLIENTS
#define __H_CLIENTS

#include <netinet/in.h>

typedef struct thread_client_t {
    char *ip;
    int port;
    int sock;
    char *nickname;
} ChatClient;

void clients_init(void);

ChatClient* clients_new(struct sockaddr_in client_addr, int socket);
void clients_add(ChatClient *client);

void clients_print_total(void);
void clients_register(ChatClient *client, char *nickname);
void clients_broadcast_message(char *from, char *message);
void clients_end_session(ChatClient *client);

#endif