/*
 * stream_parser.c
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

#include <stdio.h>
#include <string.h>
#include <expat.h>
#include <glib.h>

#include "server/stream_parser.h"
#include "server/stanza.h"
#include "server/stanzas.h"
#include "server/log.h"

static int depth = 0;
static int do_reset = 0;

static XML_Parser parser;
static XMPPStanza *curr_stanza;
static GString *curr_string = NULL;

static stream_start_func stream_start_cb = NULL;
static auth_func auth_cb = NULL;
static id_func id_cb = NULL;
static query_func query_cb = NULL;

static void
start_element(void *data, const char *element, const char **attributes)
{
    if (g_strcmp0(element, "stream:stream") == 0) {
        log_println("RECV: %s", curr_string->str);
        stream_start_cb();
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

    if (depth > 0) {
        stanza_add_child(curr_stanza->parent, curr_stanza);
        curr_stanza = curr_stanza->parent;
    } else {
        log_println("RECV: %s", curr_string->str);
        stanza_add(curr_stanza);
        if (stanza_get_child_by_ns(curr_stanza, "jabber:iq:auth")) {
            auth_cb(curr_stanza);
        } else {
            const char *id = stanza_get_id(curr_stanza);
            if (id) {
                id_cb(id);
            }
            const char *query = stanza_get_query_request(curr_stanza);
            if (query) {
                query_cb(query, id);
            }
        }

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
parser_init(stream_start_func startcb, auth_func authcb, id_func idcb, query_func querycb)
{
    if (curr_string) {
        g_string_free(curr_string, TRUE);
    }
    curr_string = g_string_new("");

    stream_start_cb = startcb;
    auth_cb = authcb;
    id_cb = idcb;
    query_cb = querycb;

    parser = XML_ParserCreate(NULL);
    XML_SetElementHandler(parser, start_element, end_element);
    XML_SetCharacterDataHandler(parser, handle_data);
}

int
parser_feed(char *chunk, int len)
{
    g_string_append_len(curr_string, chunk, len);
    int res = XML_Parse(parser, chunk, len, 0);
    parser_reset();
    return res;
}

void
parser_close(void)
{
    XML_ParserFree(parser);
    parser = NULL;
}

void
parser_reset(void)
{
    if (do_reset == 1) {
        parser_close();
        parser_init(stream_start_cb, auth_cb, id_cb, query_cb);
        do_reset = 0;
    }
}
