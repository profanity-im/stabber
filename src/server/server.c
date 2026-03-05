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

#include <config.h>

#include <string.h>
#include <glib.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#ifdef PLATFORM_OSX
#include <pthread.h>
#else
#include <sys/prctl.h>
#endif

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
static gboolean server_initialized = FALSE;

void
server_init(void)
{
    if (server_initialized) {
        return;
    }
    pthread_mutex_init(&send_queue_lock, NULL);
    stanzas_init();
    server_initialized = TRUE;
}

static GList *send_queue;
static XMPPClient *client;
static int listen_socket = -1;
static pthread_t server_thread;
static gboolean server_thread_started = FALSE;
static gboolean kill_recv = FALSE;
static gboolean httpapi_run = FALSE;

static void _shutdown(void);
static void* _start_server_cb(void* userdata);

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
                log_println(STBBR_LOGERROR, "Error sending on connection: %s", strerror(errno));
                return;
            }
        }

        to_send -= sent;
        marker += sent;
    }

    log_println(STBBR_LOGINFO, "SENT: %s", stream);
}

int
read_stream(void)
{
    char buf[4096];
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

        int read_size = recv(client->sock, buf, sizeof(buf) - 1, 0);

        // client disconnect
        if (read_size == 0) {
            log_println(STBBR_LOGINFO, "%s:%d - Client disconnected.", client->ip, client->port);
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
                log_println(STBBR_LOGERROR, "Error receiving on connection: %s", strerror(errno));
                g_string_free(stream, TRUE);
                return -1;
            }
        }

        buf[read_size] = '\0';
        parser_feed(buf, read_size);
        g_string_append_len(stream, buf, read_size);

        if (g_str_has_suffix(stream->str, STREAM_END)) {
            log_println(STBBR_LOGINFO, "RECV: </stream:stream>");
            log_println(STBBR_LOGINFO, "--> Stream end callback fired");
            write_stream(STREAM_END);
            kill_recv = TRUE;
        }
    }

    return 0;
}

void
stream_start_callback(void)
{
    log_println(STBBR_LOGINFO, "--> Stream start callback fired");

    write_stream(XML_START);
    write_stream(STREAM_RESP);
    write_stream(FEATURES);
}

void
auth_callback(XMPPStanza *stanza)
{
    log_println(STBBR_LOGINFO, "--> Auth callback fired");

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
        log_println(STBBR_LOGDEBUG, "Auth attempt: user=%s, pass=%s, res=%s", client->username, client->password, client->resource);

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
id_callback(const char *name, const char *id)
{
    char *stream = prime_get_for_id(id);
    if (!stream) {
        // Try matching with name:id
        gchar *combined = g_strdup_printf("%s:%s", name, id);
        stream = prime_get_for_id(combined);
        g_free(combined);
    }

    if (!stream) {
        return;
    }

    log_println(STBBR_LOGINFO, "--> ID callback fired for '%s'", id);

    XMPPStanza *stanza = stanza_parse(stream);
    stanza_set_id(stanza, id);
    char *out = stanza_to_string(stanza);
    write_stream(out);
    free(out);
    stanza_free(stanza);
}

void
query_callback(const char *query, const char *id)
{
    XMPPStanza *stub = prime_get_for_query(query);
    if (!stub) {
        return;
    }

    log_println(STBBR_LOGINFO, "--> QUERY callback fired for '%s'", query);
    
    // Clone the stub so we don't modify the global one
    char *stub_str = stanza_to_string(stub);
    XMPPStanza *stanza = stanza_parse(stub_str);
    free(stub_str);

    stanza_set_id(stanza, id);
    char *stream = stanza_to_string(stanza);
    write_stream(stream);
    free(stream);
    stanza_free(stanza);
}

void
xmlns_callback(const char *xmlns, const char *id)
{
    XMPPStanza *stub = prime_get_for_xmlns(xmlns);
    if (!stub) {
        return;
    }

    log_println(STBBR_LOGINFO, "--> XMLNS callback fired for '%s'", xmlns);
    
    // Clone the stub so we don't modify the global one
    char *stub_str = stanza_to_string(stub);
    XMPPStanza *stanza = stanza_parse(stub_str);
    free(stub_str);

    stanza_set_id(stanza, id);
    char *stream = stanza_to_string(stanza);
    write_stream(stream);
    free(stream);
    stanza_free(stanza);
}

void
server_wait_for(char *id)
{
    server_init();
    log_println(STBBR_LOGINFO, "Received wait for stanza with id: %s", id);
    
    int retries = 0;
    while (retries < 500) { // 5 seconds (500 * 10ms)
        int res = stanzas_contains_id(id);
        if (res) {
            log_println(STBBR_LOGINFO, "WAIT complete for id: %s", id);
            return;
        }
        usleep(1000 * 10);
        retries++;
    }
    log_println(STBBR_LOGERROR, "WAIT TIMEOUT for id: %s", id);
}

int
server_run(stbbr_log_t loglevel, int *port, int httpport)
{
    usleep(1000 * 100); // 100ms delay to let OS release port
    log_pre_init();
    server_init();
    
    usleep(1000 * 1000); // 1s wait for OS to truly release previous port

#ifdef PLATFORM_OSX
    pthread_setname_np("main");
#else
    prctl(PR_SET_NAME, "main");
#endif

    pthread_mutex_lock(&send_queue_lock);
    send_queue = NULL;
    pthread_mutex_unlock(&send_queue_lock);

    kill_recv = FALSE;
    client = NULL;
    verify_set_timeout(10);

    log_init(loglevel);
    log_println(STBBR_LOGINFO, "Starting on port: %d...", *port);

    // create listen socket
    errno = 0;
    if (listen_socket < 0) {
        listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
        if (listen_socket == -1) {
            log_println(STBBR_LOGERROR, "Could not create socket: %s", strerror(errno));
            _shutdown();
            return -1;
        }

        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(*port);

        int reuse = 1;
        setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
#ifdef SO_REUSEPORT
        setsockopt(listen_socket, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse));
#endif

        // bind socket to port
        errno = 0;
        int ret = bind(listen_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));
        if (ret == -1) {
            log_println(STBBR_LOGERROR, "Bind failed: %s", strerror(errno));
            _shutdown();
            return -1;
        }

        // If port 0 was used, get the assigned port
        if (*port == 0) {
            socklen_t len = sizeof(server_addr);
            if (getsockname(listen_socket, (struct sockaddr *)&server_addr, &len) == -1) {
                log_println(STBBR_LOGERROR, "getsockname failed: %s", strerror(errno));
                _shutdown();
                return -1;
            }
            *port = ntohs(server_addr.sin_port);
        }

        // set socket to listen mode
        errno = 0;
        ret = listen(listen_socket, 5);
        if (ret == -1) {
            log_println(STBBR_LOGERROR, "Listen failed: %s", strerror(errno));
            _shutdown();
            return -1;
        }
    }

    prime_init();

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 1024 * 1024); // 1MB

    // start client processor thread
    int res = pthread_create(&server_thread, &attr, _start_server_cb, NULL);
    pthread_attr_destroy(&attr);
    if (res != 0) {
        _shutdown();
        return -1;
    }
    server_thread_started = TRUE;

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
    server_init();
    log_println(STBBR_LOGDEBUG, "Received send: %s", stream);

    pthread_mutex_lock(&send_queue_lock);
    send_queue = g_list_append(send_queue, strdup(stream));
    pthread_mutex_unlock(&send_queue_lock);
}

void
server_stop(void)
{
    server_init();
    if (kill_recv) {
        return;
    }

    log_println(STBBR_LOGINFO, "SERVER STOP");
    kill_recv = TRUE;
    if (server_thread_started) {
        pthread_join(server_thread, NULL);
        server_thread_started = FALSE;
    }

    if (listen_socket > 0) {
        shutdown(listen_socket, SHUT_RDWR);
        char junk[1024];
        while (recv(listen_socket, junk, sizeof(junk), MSG_DONTWAIT) > 0);
        close(listen_socket);
        listen_socket = -1;
    }

    _shutdown();
    kill_recv = FALSE;
}

static void*
_start_server_cb(void* userdata)
{
#ifdef PLATFORM_OSX
    pthread_setname_np("stbr");
#else
    prctl(PR_SET_NAME, "stbr");
#endif

    // listen socket non blocking
    int res = fcntl(listen_socket, F_SETFL, fcntl(listen_socket, F_GETFL, 0) | O_NONBLOCK);
    if (res == -1) {
        log_println(STBBR_LOGERROR, "Error setting nonblocking on listen socket: %s", strerror(errno));
        return NULL;
    }

    log_println(STBBR_LOGINFO, "Waiting for incoming connection...");

    // wait for connection
    int client_socket;
    errno = 0;
    while (!kill_recv && (client_socket = accept(listen_socket, NULL, NULL)) == -1) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            log_println(STBBR_LOGERROR, "Accept failed: %s", strerror(errno));
            return NULL;
        }
        errno = 0;
        usleep(1000 * 5);
    }

    if (kill_recv) {
        return NULL;
    }

    // client socket non blocking
    res = fcntl(client_socket, F_SETFL, fcntl(client_socket, F_GETFL, 0) | O_NONBLOCK);
    if (res == -1) {
        log_println(STBBR_LOGERROR, "Error setting nonblocking on client socket: %s", strerror(errno));
        return NULL;
    }

    struct sockaddr_in dummy_addr;
    memset(&dummy_addr, 0, sizeof(dummy_addr));
    client = xmppclient_new(dummy_addr, client_socket);
    parser_init(stream_start_callback, auth_callback, id_callback, query_callback, xmlns_callback);

    read_stream();

    return NULL;
}

static void
_shutdown(void)
{
    log_println(STBBR_LOGINFO, "SHUTDOWN");

    if (httpapi_run) {
        httpapi_stop();
        httpapi_run = FALSE;
    }

    if (client) {
        xmppclient_end_session(client);
        client = NULL;
    }

    parser_close();

    prime_free_all();
    stanzas_free_all();

    pthread_mutex_lock(&send_queue_lock);
    g_list_free_full(send_queue, free);
    send_queue = NULL;
    pthread_mutex_unlock(&send_queue_lock);

    log_println(STBBR_LOGINFO, "SHUTDOWN DONE");
    log_close();
}
