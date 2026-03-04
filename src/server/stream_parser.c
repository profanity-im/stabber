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

static XML_Parser parser;
static XMPPStanza *curr_stanza;
static GString *curr_string = NULL;

static stream_start_func stream_start_cb = NULL;
static auth_func auth_cb = NULL;
static id_func id_cb = NULL;
static query_func query_cb = NULL;

static void _start_element(void *data, const char *element, const char **attributes);
static void _end_element(void *data, const char *element);
static void _handle_data(void *data, const char *content, int length);

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
    XML_SetElementHandler(parser, _start_element, _end_element);
    XML_SetCharacterDataHandler(parser, _handle_data);
    depth = 0;
}

int
parser_feed(char *chunk, int len)
{
    int res = XML_Parse(parser, chunk, len, 0);
    return res;
}

void
parser_close(void)
{
    if (parser) {
        XML_ParserFree(parser);
        parser = NULL;
    }
}

void
parser_reset(void)
{
    // No-op for continuous XMPP stream
}

static void
_start_element(void *data, const char *element, const char **attributes)
{
    if (g_strcmp0(element, "stream:stream") == 0) {
        log_println(STBBR_LOGINFO, "RECV: <stream:stream>");
        stream_start_cb();
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
_end_element(void *data, const char *element)
{
    depth--;
    
    if (depth > 1) {
        stanza_add_child(curr_stanza->parent, curr_stanza);
        curr_stanza = curr_stanza->parent;
        return;
    }

    if (depth == 1) {
        log_println(STBBR_LOGINFO, "RECV stanza: %s", element);
        stanzas_add(curr_stanza);
        
        if (stanza_get_child_by_ns(curr_stanza, "jabber:iq:auth")) {
            auth_cb(curr_stanza);
        } else {
            const char *id = stanza_get_id(curr_stanza);
            if (id) {
                id_cb(curr_stanza->name, id);
            }
            const char *query = stanza_get_query_request(curr_stanza);
            if (query) {
                query_cb(query, id);
            }
        }
        
        curr_stanza = curr_stanza->parent;
    }
}

static void
_handle_data(void *data, const char *content, int length)
{
    if (depth > 1) {
        if (!curr_stanza->content) {
            curr_stanza->content = g_string_new("");
        }
        g_string_append_len(curr_stanza->content, content, length);
    }
}
