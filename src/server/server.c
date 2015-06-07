/*
 * server.c
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

#include <string.h>
#include <glib.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/prctl.h>

#include "server/xmppclient.h"
#include "server/stream_parser.h"
#include "server/prime.h"
#include "server/stanza.h"
#include "server/stanzas.h"
#include "server/verify.h"
#include "server/server.h"
#include "server/httpapi.h"
#include "server/log.h"

#define XML_START "<?xml version=\"1.0\"?>"

#define STREAM_RESP  "<stream:stream from=\"localhost\" id=\"stream1\" xml:lang=\"en\" version=\"1.0\" xmlns=\"jabber:client\" xmlns:stream=\"http://etherx.jabber.org/streams\">"
#define FEATURES "<stream:features></stream:features>"

#define STREAM_END "</stream:stream>"

pthread_mutex_t send_queue_lock;
static GList *send_queue;

static XMPPClient *client;

static void _shutdown(void);

static int listen_socket;
static pthread_t server_thread;
static gboolean kill_recv = FALSE;
static gboolean httpapi_run = FALSE;

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
        if (kill_recv) {
            _shutdown();
            return 0;
        }

        // send anything from queue
        pthread_mutex_lock(&send_queue_lock);
        GList *curr_send = send_queue;
        while (curr_send) {
            write_stream(curr_send->data);
            curr_send = g_list_next(curr_send);
        }

        g_list_free_full(send_queue, free);
        send_queue = NULL;
        pthread_mutex_unlock(&send_queue_lock);

        int read_size = recv(client->sock, buf, 1, 0);

        // client disconnect
        if (read_size == 0) {
            log_println("%s:%d - Client disconnected.", client->ip, client->port);
            g_string_free(stream, TRUE);
            return -1;
        }

        // error
        if (read_size == -1) {
            // got nothing, sleep and try again
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                errno = 0;
                usleep(1000 * 5);
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
            log_println("STREAM END");
            kill_recv = TRUE;
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

    const char *id = stanza_get_id(stanza);
    XMPPStanza *query = stanza_get_child_by_ns(stanza, "jabber:iq:auth");
    XMPPStanza *username = stanza_get_child_by_name(query, "username");
    XMPPStanza *password = stanza_get_child_by_name(query, "password");
    XMPPStanza *resource = stanza_get_child_by_name(query, "resource");

    if (!username || !password || !resource) {
        GString *authfields = g_string_new("<iq type=\"result\" id=\"");
        g_string_append(authfields, id);
        g_string_append(authfields, "\"><query xmlns=\"jabber:iq:auth\">");
        if (!username) {
            g_string_append(authfields, "<username/>");
        }
        if (!password) {
            g_string_append(authfields, "<password/>");
        }
        if (!resource) {
            g_string_append(authfields, "<resource/>");
        }
        g_string_append(authfields, "</query></iq>");
        write_stream(authfields->str);
        g_string_free(authfields, TRUE);
    } else {
        client->username = strdup(username->content->str);
        client->password = strdup(password->content->str);
        client->resource = strdup(resource->content->str);

        char *expected_password = prime_get_passwd();
        if (g_strcmp0(client->password, expected_password) != 0) {
            GString *authfail = g_string_new("<iq id=\"");
            g_string_append(authfail, id);
            g_string_append(authfail, "\" type=\"error\"/>");
            write_stream(authfail->str);
            g_string_free(authfail, TRUE);
            write_stream(STREAM_END);
            return;
        }

        GString *authsuccess = g_string_new("<iq id=\"");
        g_string_append(authsuccess, id);
        g_string_append(authsuccess, "\" type=\"result\"/>");
        write_stream(authsuccess->str);
        g_string_free(authsuccess, TRUE);
    }
}

void
id_callback(const char *id)
{
    char *stream = prime_get_for_id(id);
    if (stream) {
        log_println("--> ID callback fired for '%s'", id);
        write_stream(stream);
    }
}

void
query_callback(const char *query, const char *id)
{
    XMPPStanza *stanza = prime_get_for_query(query);
    if (stanza) {
        log_println("--> QUERY callback fired for '%s'", query);
        stanza_set_id(stanza, id);
        char *stream = stanza_to_string(stanza);
        write_stream(stream);
        free(stream);
    }
}

void
server_wait_for(char *id)
{
    log_println("WAIT for stanza with id: %s", id);
    while (TRUE) {
        int res = stanzas_contains_id(id);
        if (res) {
            log_println("WAIT complete for id: %s", id);
            return;
        }
        usleep(1000 * 5);
    }
}

void*
_start_server_cb(void* userdata)
{
    prctl(PR_SET_NAME, "stbbr");

    struct sockaddr_in client_addr;

    // listen socket non blocking
    int res = fcntl(listen_socket, F_SETFL, fcntl(listen_socket, F_GETFL, 0) | O_NONBLOCK);
    if (res == -1) {
        log_println("Error setting nonblocking on listen socket: %s", strerror(errno));
        return NULL;
    }

    log_println("Waiting for incoming connection...");

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
        usleep(1000 * 5);
    }

    // client socket non blocking
    res = fcntl(client_socket, F_SETFL, fcntl(client_socket, F_GETFL, 0) | O_NONBLOCK);
    if (res == -1) {
        log_println("Error setting nonblocking on client socket: %s", strerror(errno));
        return NULL;
    }

    client = xmppclient_new(client_addr, client_socket);
    parser_init(stream_start_callback, auth_callback, id_callback, query_callback);

    read_stream();

    return NULL;
}

int
server_run(int port, int httpport)
{
    pthread_mutex_lock(&send_queue_lock);
    send_queue = NULL;
    pthread_mutex_unlock(&send_queue_lock);

    kill_recv = FALSE;
    client = NULL;
    verify_set_timeout(10);

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

    prime_init();

    // start client processor thread
    int res = pthread_create(&server_thread, NULL, _start_server_cb, NULL);
    if (res != 0) {
        _shutdown();
        return -1;
    }

    // start http server
    if (httpport > 0) {
        res = httpapi_start(httpport);
        if (!res) {
            _shutdown();
            return -1;
        }
        httpapi_run = TRUE;
    }

    return 0;
}

void
server_send(char *stream)
{
    pthread_mutex_lock(&send_queue_lock);
    send_queue = g_list_append(send_queue, strdup(stream));
    pthread_mutex_unlock(&send_queue_lock);
}

void
server_stop(void)
{
    if (!kill_recv) {
        log_println("SERVER STOP");
        kill_recv = TRUE;
        pthread_join(server_thread, NULL);
    }
}

static void
_shutdown(void)
{
    log_println("SHUTDOWN");

    if (httpapi_run) {
        httpapi_stop();
    }

    xmppclient_end_session(client);
    client = NULL;

    parser_close();

    shutdown(listen_socket, 2);
    while (recv(listen_socket, NULL, 1, 0) > 0) {}
    close(listen_socket);

    prime_free_all();
    stanzas_free_all();

    pthread_mutex_lock(&send_queue_lock);
    g_list_free_full(send_queue, free);
    send_queue = NULL;
    pthread_mutex_unlock(&send_queue_lock);

    log_println("");
    log_println("");
    log_close();
}
