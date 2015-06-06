/*
 * stanza.c
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
#include <expat.h>
#include <glib.h>

#include "server/stanza.h"
#include "server/stanzas.h"

typedef struct parse_state_t {
    int depth;
    XMPPStanza *curr_stanza;
} ParseState;

static void
start_element(void *data, const char *element, const char **attributes)
{
    ParseState *state = data;

    XMPPStanza *stanza = stanza_new(element, attributes);
    if (state->depth == 0) {
        state->curr_stanza = stanza;
        state->curr_stanza->parent = NULL;
    } else {
        stanza->parent = state->curr_stanza;
        state->curr_stanza = stanza;
    }

    state->depth++;
}

static void
end_element(void *data, const char *element)
{
    ParseState *state = data;

    state->depth--;

    if (state->depth > 0) {
        stanza_add_child(state->curr_stanza->parent, state->curr_stanza);
        state->curr_stanza = state->curr_stanza->parent;
    }
}

static void
handle_data(void *data, const char *content, int length)
{
    ParseState *state = data;

    if (!state->curr_stanza->content) {
        state->curr_stanza->content = g_string_new("");
    }

    g_string_append_len(state->curr_stanza->content, content, length);
}

XMPPStanza *
parse_stanza(char *stanza_text)
{
    ParseState *state = malloc(sizeof(ParseState));
    state->depth = 0;
    state->curr_stanza = NULL;

    XML_Parser parser = XML_ParserCreate(NULL);
    XML_SetElementHandler(parser, start_element, end_element);
    XML_SetCharacterDataHandler(parser, handle_data);
    XML_SetUserData(parser, state);

    XML_Parse(parser, stanza_text, strlen(stanza_text), 0);
    XML_ParserFree(parser);

    return state->curr_stanza;
}
