#include <string.h>
#include <glib.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "server/clients.h"

#define STREAM_REQ "<?xml version=\"1.0\"?><stream:stream to=\"localhost\" xml:lang=\"en\" version=\"1.0\" xmlns=\"jabber:client\" xmlns:stream=\"http://etherx.jabber.org/streams\">"
#define STREAM_RESP  "<?xml version=\"1.0\"?><stream:stream from=\"localhost\" id=\"stream1\" xml:lang=\"en\" version=\"1.0\" xmlns=\"jabber:client\" xmlns:stream=\"http://etherx.jabber.org/streams\">"

void* connection_handler(void *data)
{
    ChatClient *client = (ChatClient *)data;
    int read_size;

    // client loop
    while(1) {
        char buf[2];
        memset(buf, 0, sizeof(buf));

        // listen to client stream
        GString *stream = g_string_new("");
        errno = 0;
        gboolean stream_opened = FALSE;
        while ((!stream_opened) && ((read_size = recv(client->sock, buf, 1, 0)) > 0)) {
            g_string_append_len(stream, buf, read_size);
            if (g_strcmp0(stream->str, STREAM_REQ) == 0) {
                stream_opened = TRUE;
            }
            memset(buf, 0, sizeof(buf));
        }

        // error
        if (read_size == -1) {
            perror("Error receiving on connection");
            g_string_free(stream, TRUE);
            break;

        // client closed
        } else if (read_size == 0) {
            printf("%s:%d - Client disconnected.\n", client->ip, client->port);
            g_string_free(stream, TRUE);
            break;

        } else {
            printf("RECV: %s\n", stream->str);
            fflush(stdout);
            g_string_free(stream, TRUE);

            int sent = 0;
            int to_send = strlen(STREAM_RESP);
            char *marker = STREAM_RESP;
            while (to_send > 0 && ((sent = write(client->sock, marker, to_send)) > 0)) {
                to_send -= sent;
                marker += sent;
            }

            printf("SENT: %s\n", STREAM_RESP);
            fflush(stdout);

            memset(buf, 0, sizeof(buf));

            // listen to client stream
            stream = g_string_new("");
            errno = 0;
            while ((read_size = recv(client->sock, buf, 1, 0)) > 0) {
                printf("%c", buf[0]);
                fflush(stdout);
                g_string_append_len(stream, buf, read_size);
                memset(buf, 0, sizeof(buf));
            }
        }
    }

    return 0;
}

int main(int argc , char *argv[])
{
    int port = 0;

    GOptionEntry entries[] =
    {
        { "port", 'p', 0, G_OPTION_ARG_INT, &port, "Listen port", NULL },
        { NULL }
    };

    GError *error = NULL;
    GOptionContext *context;

    context = g_option_context_new(NULL);
    g_option_context_add_main_entries(context, entries, NULL);
    if (!g_option_context_parse(context, &argc, &argv, &error)) {
        g_print("%s\n", error->message);
        g_option_context_free(context);
        g_error_free(error);
        return 1;
    }

    g_option_context_free(context);

    if (port == 0) {
        port = 5222;
    }

    int listen_socket, client_socket;
    int c, ret;
    struct sockaddr_in server_addr, client_addr;

    if (argc == 2) {
        port = atoi(argv[1]);
    }

    printf("Starting on port: %d...\n", port);

    clients_init();

    // create socket
    errno = 0;
    listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_IP); // ipv4, tcp, ip
    if (listen_socket == -1) {
        perror("Could not create socket");
        return 0;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // bind socket to port
    errno = 0;
    ret = bind(listen_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (ret == -1) {
        perror("Bind failed");
        return 0;
    }

    // set socket to listen mode
    errno = 0;
    ret = listen(listen_socket, 5);
    if (ret == -1) {
        perror("Listen failed");
        return 0;
    }
    puts("Waiting for incoming connections...");

    // connection accept loop
    while (1) {
        c = sizeof(struct sockaddr_in);
        errno = 0;
        client_socket = accept(listen_socket, (struct sockaddr *)&client_addr, (socklen_t*)&c);
        if (client_socket == -1) {
            perror("Accept failed");
        }

        // create thread for each new client
        ChatClient *client = clients_new(client_addr, client_socket);
        clients_add(client);

        pthread_t client_thread;
        ret = pthread_create(&client_thread, NULL, connection_handler, (void *)client);
        if (ret == 0) {
            printf("%s:%d - Connection handler assigned.\n", client->ip, client->port);
        } else {
            puts("Could not create thread.");
        }

        clients_print_total();
    }

    return 0;
}
