#include <stdio.h>
#include <string.h>
#include <expat.h>
#include <glib.h>

#include "server/parser.h"
#include "server/xmppclient.h"
#include "server/stanza.h"

static int depth = 0;
static int do_reset = 0;

static XML_Parser parser;
static XMPPClient *xmppclient;
static XMPPStanza *curr_stanza;

static stream_start_func stream_start_cb = NULL;
static auth_func auth_cb = NULL;

static void
start_element(void *data, const char *element, const char **attributes)
{
    if (g_strcmp0(element, "stream:stream") == 0) {
        stream_start_cb(xmppclient);
        do_reset = 1;
        return;
    }

    XMPPStanza *stanza = stanza_new(element, attributes);

    if (depth == 0) {
        curr_stanza = stanza;
        curr_stanza->parent = NULL;
    } else {
        stanza->parent = curr_stanza;
        curr_stanza = stanza;
    }

    depth++;
}

static void
end_element(void *data, const char *element)
{
    depth--;

    if (stanza_get_child_by_ns(curr_stanza, "jabber:iq:auth")) {
        auth_cb(curr_stanza, xmppclient);
    }

    if (depth > 0) {
        stanza_add_child(curr_stanza->parent, curr_stanza);
        curr_stanza = curr_stanza->parent;
    } else {
        stanza_add(curr_stanza);
    }

    if (depth == 0) {
        do_reset = 1;
    }
}

static void
handle_data(void *data, const char *content, int length)
{
    if (!curr_stanza->content) {
        curr_stanza->content = g_string_new("");
    }

    g_string_append_len(curr_stanza->content, content, length);
}

void
parser_init(XMPPClient *client, stream_start_func startcb, auth_func authcb)
{
    xmppclient = client;
    stream_start_cb = startcb;
    auth_cb = authcb;

    parser = XML_ParserCreate(NULL);
    XML_SetElementHandler(parser, start_element, end_element);
    XML_SetCharacterDataHandler(parser, handle_data);
}

int
parser_feed(char *chunk, int len)
{
    return XML_Parse(parser, chunk, len, 0);
}

void
parser_close(void)
{
    XML_ParserFree(parser);
}

void
parser_reset(void)
{
    if (do_reset == 1) {
        parser_close();
        parser_init(xmppclient, stream_start_cb, auth_cb);
        do_reset = 0;
    }
}
