#include <string.h>
#include <glib.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "server/xmppclient.h"
#include "server/parser.h"

#define XML_START "<?xml version=\"1.0\"?>"
#define STREAM_REQ "<stream:stream to=\"localhost\" xml:lang=\"en\" version=\"1.0\" xmlns=\"jabber:client\" xmlns:stream=\"http://etherx.jabber.org/streams\">"
#define STREAM_RESP  "<?xml version=\"1.0\"?><stream:stream from=\"localhost\" id=\"stream1\" xml:lang=\"en\" version=\"1.0\" xmlns=\"jabber:client\" xmlns:stream=\"http://etherx.jabber.org/streams\">"
#define FEATURES "<stream:features></stream:features>"
#define AUTH_REQ "<iq id=\"_xmpp_auth1\" type=\"set\"><query xmlns=\"jabber:iq:auth\"><username>stabber</username><password>password</password><resource>profanity</resource></query></iq>"
#define AUTH_RESP "<iq id=\"_xmpp_auth1\" type=\"result\"/>"
#define END_STREAM "</stream:stream>"

int
listen_for_xmlstart(XMPPClient *client)
{
    int read_size;
    char buf[2];
    memset(buf, 0, sizeof(buf));

    GString *stream = g_string_new("");
    errno = 0;
    while ((read_size = recv(client->sock, buf, 1, 0)) > 0) {
        g_string_append_len(stream, buf, read_size);
        memset(buf, 0, sizeof(buf));
        if (g_strcmp0(stream->str, XML_START) == 0) {
            break;
        }
    }

    // error
    if (read_size == -1) {
        perror("Error receiving on connection");
        xmppclient_end_session(client);
        g_string_free(stream, TRUE);
        return -1;

    // client closed
    } else if (read_size == 0) {
        printf("\n%s:%d - Client disconnected.\n", client->ip, client->port);
        xmppclient_end_session(client);
        g_string_free(stream, TRUE);
        return -1;
    }

    printf("RECV: %s\n", XML_START);
    fflush(stdout);
    return 0;
}

int
listen_for(XMPPClient *client, const char * const stanza)
{
    int read_size;
    char buf[2];
    memset(buf, 0, sizeof(buf));

    GString *stream = g_string_new("");
    errno = 0;
    while ((read_size = recv(client->sock, buf, 1, 0)) > 0) {
        g_string_append_len(stream, buf, read_size);
        memset(buf, 0, sizeof(buf));
        if (g_strcmp0(stream->str, stanza) == 0) {
            break;
        }
    }

    // error
    if (read_size == -1) {
        perror("Error receiving on connection");
        xmppclient_end_session(client);
        g_string_free(stream, TRUE);
        return -1;

    // client closed
    } else if (read_size == 0) {
        printf("\n%s:%d - Client disconnected.\n", client->ip, client->port);
        xmppclient_end_session(client);
        g_string_free(stream, TRUE);
        return -1;
    }

    printf("RECV: %s\n", stanza);
    fflush(stdout);
    return 0;
}

void
send_to(XMPPClient *client, const char * const stanza)
{
    int sent = 0;
    int to_send = strlen(stanza);
    char *marker = (char*)stanza;
    while (to_send > 0 && ((sent = write(client->sock, marker, to_send)) > 0)) {
        to_send -= sent;
        marker += sent;
    }
    printf("SENT: %s\n", stanza);
    fflush(stdout);
}

int
debug_client(XMPPClient *client)
{
    int read_size;
    char buf[2];
    memset(buf, 0, sizeof(buf));

    GString *stream = g_string_new("");
    errno = 0;
    while ((read_size = recv(client->sock, buf, 1, 0)) > 0) {
        parser_feed(buf, 1);
        fflush(stdout);
        parser_reset();
        g_string_append_len(stream, buf, read_size);
        memset(buf, 0, sizeof(buf));
        if (g_str_has_suffix(stream->str, END_STREAM)) {
            break;
        }
    }

    // error
    if (read_size == -1) {
        perror("Error receiving on connection");
        xmppclient_end_session(client);
        g_string_free(stream, TRUE);
        return -1;

    // client closed
    } else if (read_size == 0) {
        printf("\n%s:%d - Client disconnected.\n", client->ip, client->port);
        xmppclient_end_session(client);
        g_string_free(stream, TRUE);
        return -1;
    }

    return 0;
}

void connection_handler(XMPPClient *client)
{
    int res = listen_for_xmlstart(client);
    if (res == -1) {
        return;
    }

    res = listen_for(client, STREAM_REQ);
    if (res == -1) {
        return;
    }

    send_to(client, STREAM_RESP);
    send_to(client, FEATURES);

    res = listen_for(client, AUTH_REQ);
    if (res == -1) {
        return;
    }

    send_to(client, AUTH_RESP);

    res = debug_client(client);
    if (res == -1) {
        return;
    }

    printf("\nEnd stream receieved\n");
    xmppclient_end_session(client);
    fflush(stdout);
    return;
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

    struct sockaddr_in server_addr, client_addr;

    if (argc == 2) {
        port = atoi(argv[1]);
    }

    printf("Starting on port: %d...\n", port);

    // create socket
    errno = 0;
    int listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_IP); // ipv4, tcp, ip
    if (listen_socket == -1) {
        perror("Could not create socket");
        return 0;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // bind socket to port
    errno = 0;
    int ret = bind(listen_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));
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

    parser_init();

    puts("Waiting for incoming connections...");

    // connection accept
    int c = sizeof(struct sockaddr_in);
    errno = 0;
    int client_socket = accept(listen_socket, (struct sockaddr *)&client_addr, (socklen_t*)&c);
    if (client_socket == -1) {
        perror("Accept failed");
        return 0;
    }

    XMPPClient *client = xmppclient_new(client_addr, client_socket);

    connection_handler(client);

    parser_close();

    while (recv(listen_socket, NULL, 1, 0) > 0) {}
    shutdown(listen_socket, 2);
    close(listen_socket);
    return 0;
}
