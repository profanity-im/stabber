#include <string.h>
#include <glib.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "server/xmppclient.h"
#include "server/parser.h"
#include "server/prime.h"
#include "server/stanza.h"
#include "server/server.h"
#include "server/log.h"

#define XML_START "<?xml version=\"1.0\"?>"

#define STREAM_RESP  "<stream:stream from=\"localhost\" id=\"stream1\" xml:lang=\"en\" version=\"1.0\" xmlns=\"jabber:client\" xmlns:stream=\"http://etherx.jabber.org/streams\">"
#define FEATURES "<stream:features></stream:features>"

#define AUTH_RESP "<iq id=\"_xmpp_auth1\" type=\"result\"/>"
#define AUTH_FAIL "<iq id=\"_xmpp_auth1\" type=\"error\"/>"

#define STREAM_END "</stream:stream>"

static XMPPClient *client;

static void _shutdown(void);

static int listen_socket;
static pthread_t server_thread;

void
write_stream(const char * const stream)
{
    int to_send = strlen(stream);
    char *marker = (char*)stream;

    while (to_send > 0) {
        int sent = write(client->sock, marker, to_send);

        // error
        if (sent == -1) {
            // write timeout, try again
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                errno = 0;
                continue;

            // real error
            } else {
                log_println("Error sending on connection: %s", strerror(errno));
                return;
            }
        }

        to_send -= sent;
        marker += sent;
    }

    log_println("SENT: %s", stream);
}

int
read_stream(void)
{
    char buf[2];
    memset(buf, 0, sizeof(buf));
    GString *stream = g_string_new("");

    errno = 0;
    while (TRUE) {
        int read_size = recv(client->sock, buf, 1, 0);

        // client disconnect
        if (read_size == 0) {
            log_println("%s:%d - Client disconnected.", client->ip, client->port);
            g_string_free(stream, TRUE);
            return -1;
        }

        // error
        if (read_size == -1) {
            // read timeout, try again
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                errno = 0;
                continue;

            // real error
            } else {
                log_println("Error receiving on connection: %s", strerror(errno));
                g_string_free(stream, TRUE);
                return -1;
            }
        }

        // success, feed parser with byte
        parser_feed(buf, 1);
        g_string_append_len(stream, buf, read_size);
        if (g_str_has_suffix(stream->str, STREAM_END)) {
            log_println("RECV: </stream:stream>");
            log_println("--> Stream end callback fired");
            write_stream(STREAM_END);
            log_println("");
            log_println("");
            _shutdown();
            return 0;
        }
        memset(buf, 0, sizeof(buf));
    }

    return 0;
}

void
stream_start_callback(void)
{
    log_println("--> Stream start callback fired");

    write_stream(XML_START);
    write_stream(STREAM_RESP);
    write_stream(FEATURES);
}

void
auth_callback(XMPPStanza *stanza)
{
    log_println("--> Auth callback fired");

    XMPPStanza *query = stanza_get_child_by_ns(stanza, "jabber:iq:auth");
    XMPPStanza *username = stanza_get_child_by_name(query, "username");
    XMPPStanza *password = stanza_get_child_by_name(query, "password");
    XMPPStanza *resource = stanza_get_child_by_name(query, "resource");

    client->username = strdup(username->content->str);
    client->password = strdup(password->content->str);
    client->resource = strdup(resource->content->str);

    char *expected_password = prime_get_passwd();
    if (g_strcmp0(client->password, expected_password) != 0) {
        write_stream(AUTH_FAIL);
        write_stream(STREAM_END);
        return;
    }

    write_stream(AUTH_RESP);
}

void
id_callback(const char *id)
{
    char *stream = prime_get_for(id);
    if (stream) {
        log_println("--> ID callback fired for '%s'", id);
        write_stream(stream);
    }
}

void*
_start_server_cb(void* userdata)
{
    struct sockaddr_in client_addr;

    // listen socket non blocking
    int res = fcntl(listen_socket, F_SETFL, fcntl(listen_socket, F_GETFL, 0) | O_NONBLOCK);
    if (res == -1) {
        log_println("Error setting nonblocking on listen socket: %s", strerror(errno));
        return NULL;
    }

    // listen socket read timeout
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 1000 * 10;
    errno = 0;
    res = setsockopt(listen_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
    if (res < 0) {
        log_println("Error setting listen socket options: %s", strerror(errno));
        return NULL;
    }

    // wait for connection
    int c = sizeof(struct sockaddr_in);
    int client_socket;
    errno = 0;
    while ((client_socket = accept(listen_socket, (struct sockaddr *)&client_addr, (socklen_t*)&c)) == -1) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            log_println("Accept failed: %s", strerror(errno));
            return NULL;
        }
        errno = 0;
    }

    // client socket non blocking
    res = fcntl(client_socket, F_SETFL, fcntl(client_socket, F_GETFL, 0) | O_NONBLOCK);
    if (res == -1) {
        log_println("Error setting nonblocking on client socket: %s", strerror(errno));
        return NULL;
    }

    // client socket read timeout
    errno = 0;
    res = setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
    if (res < 0) {
        log_println("Error setting client socket options: %s", strerror(errno));
        return NULL;
    }

    client = xmppclient_new(client_addr, client_socket);
    parser_init(stream_start_callback, auth_callback, id_callback);

    read_stream();

    return NULL;
}

int
server_run(int port)
{
    client = NULL;
    log_init();
    log_println("Starting on port: %d...", port);

    // create listen socket
    errno = 0;
    listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (listen_socket == -1) {
        log_println("Could not create socket: %s", strerror(errno));
        _shutdown();
        return -1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    int reuse = 1;
    int ret = setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    if (ret == -1) {
        log_println("Set socket options failed: %s", strerror(errno));
        _shutdown();
        return -1;
    }

    // bind socket to port
    errno = 0;
    ret = bind(listen_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (ret == -1) {
        log_println("Bind failed: %s", strerror(errno));
        _shutdown();
        return -1;
    }

    // set socket to listen mode
    errno = 0;
    ret = listen(listen_socket, 5);
    if (ret == -1) {
        log_println("Listen failed: %s", strerror(errno));
        _shutdown();
        return -1;
    }

    log_println("Waiting for incoming connection...");

    prime_init();

    // start client processor thread
    int res = pthread_create(&server_thread, NULL, _start_server_cb, NULL);
    if (res != 0) {
        _shutdown();
        return -1;
    }

    return 0;
}

void
server_stop(void)
{
    pthread_join(server_thread, NULL);
}

static void
_shutdown(void)
{
//    stanza_show_all();
    xmppclient_end_session(client);
    client = NULL;

    parser_close();

    shutdown(listen_socket, 2);
    while (recv(listen_socket, NULL, 1, 0) > 0) {}
    close(listen_socket);

    prime_free_all();
    stanza_free_all();
    log_close();
}
