#include <string.h>
#include <glib.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "server/xmppclient.h"
#include "server/parser.h"
#include "server/stanza.h"
#include "server/server.h"
#include "server/log.h"

#define XML_START "<?xml version=\"1.0\"?>"

#define STREAM_RESP  "<stream:stream from=\"localhost\" id=\"stream1\" xml:lang=\"en\" version=\"1.0\" xmlns=\"jabber:client\" xmlns:stream=\"http://etherx.jabber.org/streams\">"
#define FEATURES "<stream:features></stream:features>"

#define AUTH_RESP "<iq id=\"_xmpp_auth1\" type=\"result\"/>"

#define STREAM_END "</stream:stream>"

int
listen_for_xmlstart(XMPPClient *client)
{
    int read_size;
    char buf[2];
    memset(buf, 0, sizeof(buf));

    GString *stream = g_string_new("");
    errno = 0;
    while ((read_size = recv(client->sock, buf, 1, 0)) > 0) {
        log_print_chars("%c", buf[0]);
        g_string_append_len(stream, buf, read_size);
        memset(buf, 0, sizeof(buf));
        if (g_strcmp0(stream->str, XML_START) == 0) {
            break;
        }
    }

    // error
    if (read_size == -1) {
        char *errmsg = strerror(errno);
        log_println("");
        log_println("Error receiving on connection: %s", errmsg);
        free(errmsg);
        xmppclient_end_session(client);
        g_string_free(stream, TRUE);
        return -1;

    // client closed
    } else if (read_size == 0) {
        log_println("");
        log_println("%s:%d - Client disconnected.", client->ip, client->port);
        xmppclient_end_session(client);
        g_string_free(stream, TRUE);
        return -1;
    }

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
    log_println("SENT: %s", stanza);
}

void
stream_end(XMPPClient *client)
{
    log_print_chars("\n");
    log_println("--> Stream end callback fired");
    send_to(client, STREAM_END);
}

int
listen_to(XMPPClient *client)
{
    int read_size;
    char buf[2];
    memset(buf, 0, sizeof(buf));

    GString *stream = g_string_new("");
    errno = 0;
    while ((read_size = recv(client->sock, buf, 1, 0)) > 0) {
        log_print_chars("%c", buf[0]);
        parser_feed(buf, 1);
        parser_reset();
        g_string_append_len(stream, buf, read_size);
        if (g_str_has_suffix(stream->str, STREAM_END)) {
            stream_end(client);
            break;
        }
        memset(buf, 0, sizeof(buf));
    }

    // error
    if (read_size == -1) {
        char *errmsg = strerror(errno);
        log_println("");
        log_println("Error receiving on connection: %s", errmsg);
        free(errmsg);
        xmppclient_end_session(client);
        g_string_free(stream, TRUE);
        return -1;

    // client closed
    } else if (read_size == 0) {
        log_println("");
        log_println("%s:%d - Client disconnected.", client->ip, client->port);
        xmppclient_end_session(client);
        g_string_free(stream, TRUE);
        return -1;
    }

    return 0;
}

void
stream_start_callback(XMPPClient *client)
{
    log_print_chars("\n");
    log_println("--> Stream start callback fired");
    send_to(client, XML_START);
    send_to(client, STREAM_RESP);
    send_to(client, FEATURES);
    log_print("RECV: ");
}

void
auth_callback(XMPPStanza *stanza, XMPPClient *client)
{
    log_print_chars("\n");
    log_println("--> Auth callback fired");
    XMPPStanza *query = stanza_get_child_by_ns(stanza, "jabber:iq:auth");
    XMPPStanza *username = stanza_get_child_by_name(query, "username");
    XMPPStanza *password = stanza_get_child_by_name(query, "password");
    XMPPStanza *resource = stanza_get_child_by_name(query, "resource");

    client->username = strdup(username->content->str);
    client->password = strdup(password->content->str);
    client->resource = strdup(resource->content->str);

    send_to(client, AUTH_RESP);
    log_print("RECV: ");
}

int
server_run(int port)
{
    log_init();
    log_println("Starting on port: %d...", port);

    struct sockaddr_in server_addr, client_addr;

    // create socket
    errno = 0;
    int listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_IP); // ipv4, tcp, ip
    if (listen_socket == -1) {
        char *errmsg = strerror(errno);
        log_println("Could not create socket: %s", errmsg);
        free(errmsg);
        return 0;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // bind socket to port
    errno = 0;
    int ret = bind(listen_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (ret == -1) {
        char *errmsg = strerror(errno);
        log_println("Bind failed: %s", errmsg);
        free(errmsg);
        return 0;
    }

    // set socket to listen mode
    errno = 0;
    ret = listen(listen_socket, 5);
    if (ret == -1) {
        char *errmsg = strerror(errno);
        log_println("Listen failed: %s", errmsg);
        free(errmsg);
        return 0;
    }

    log_println("Waiting for incoming connections...");

    // connection accept
    int c = sizeof(struct sockaddr_in);
    errno = 0;
    int client_socket = accept(listen_socket, (struct sockaddr *)&client_addr, (socklen_t*)&c);
    if (client_socket == -1) {
        char *errmsg = strerror(errno);
        log_println("Accept failed: %s", errmsg);
        free(errmsg);
        return 0;
    }

    XMPPClient *client = xmppclient_new(client_addr, client_socket);
    parser_init(client, stream_start_callback, auth_callback);
    log_print("RECV: ");
    int res = listen_for_xmlstart(client);
    if (res == -1) {
        return 0;
    }

    log_print("RECV: ");
    listen_to(client);

    stanza_show_all();

    xmppclient_end_session(client);

    parser_close();

    while (recv(listen_socket, NULL, 1, 0) > 0) {}
    shutdown(listen_socket, 2);
    close(listen_socket);

    log_close();

    return 1;
}
