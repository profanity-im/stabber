#include <stdio.h>
#include <string.h>
#include <expat.h>
#include <glib.h>

#include "server/parser.h"
#include "server/xmppclient.h"

static int depth = 0;
static int do_reset = 0;
static XML_Parser parser;
static stream_start_func stream_start_cb = NULL;
static stream_end_func stream_end_cb = NULL;
static auth_func auth_cb = NULL;
static XMPPClient *xmppclient;

typedef struct xmpp_attr_t {
    const char *name;
    const char *value;
} XMPPAttr;

typedef struct xmpp_stanza_t {
    const char *name;
    GList *attrs;
    GList *children;
    GString *content;
    struct xmpp_stanza_t *parent;
} XMPPStanza;

static XMPPStanza *curr_stanza;
static GList *stanzas;

void
show_stanza(XMPPStanza *stanza)
{
    printf("NAME   : %s\n", stanza->name);

    if (stanza->content && stanza->content->len > 0) {
        printf("CONTENT: %s\n", stanza->content->str);
    }

    if (stanza->attrs) {
        GList *curr_attr = stanza->attrs;
        while (curr_attr) {
            XMPPAttr *attr = curr_attr->data;
            printf("ATTR   : %s='%s'\n", attr->name, attr->value);

            curr_attr = g_list_next(curr_attr);
        }
    }

    if (stanza->children) {
        printf("CHILDREN:\n");
        GList *curr_child = stanza->children;
        while (curr_child) {
            XMPPStanza *child = curr_child->data;
            show_stanza(child);
            curr_child = g_list_next(curr_child);
        }
    }
}

void
parser_show_stanzas(void)
{
    GList *curr = stanzas;
    while (curr) {
        XMPPStanza *stanza = curr->data;
        show_stanza(stanza);
        printf("\n");
        curr = g_list_next(curr);
    }
}

static void
start_element(void *data, const char *element, const char **attribute)
{
    int i;

    if (g_strcmp0(element, "stream:stream") == 0) {
        stream_start_cb(xmppclient);
        do_reset = 1;
        return;
    }

    XMPPStanza *stanza = malloc(sizeof(XMPPStanza));
    stanza->name = strdup(element);
    stanza->content = NULL;
    stanza->children = NULL;
    if (attribute[0]) {
        for (i = 0; attribute[i]; i += 2) {
            XMPPAttr *attr = malloc(sizeof(XMPPAttr));
            attr->name = strdup(attribute[i]);
            attr->value = strdup(attribute[i+1]);
            stanza->attrs = g_list_append(stanza->attrs, attr);
        }
    } else {
        stanza->attrs = NULL;
    }

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

    if (depth > 0) {
        curr_stanza->parent->children = g_list_append(curr_stanza->parent->children, curr_stanza);
        curr_stanza = curr_stanza->parent;
    } else {
        stanzas = g_list_append(stanzas, curr_stanza);
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
parser_init(XMPPClient *client, stream_start_func startcb, stream_end_func endcb, auth_func authcb)
{
    xmppclient = client;
    stream_start_cb = startcb;
    stream_end_cb = endcb;
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
        parser_init(xmppclient, stream_start_cb, stream_end_cb, auth_cb);
        do_reset = 0;
    }
}
