/*
 * verify.c
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
#include <unistd.h>

#include <expat.h>

#include "server/stanza.h"
#include "server/log.h"

static int depth = 0;
static XMPPStanza *curr_stanza = NULL;
static int timeoutsecs = 0;

void
verify_set_timeout(int seconds)
{
    if (seconds <= 0) {
        timeoutsecs = 0;
    } else {
        timeoutsecs = seconds;
    }
}

static void
start_element(void *data, const char *element, const char **attributes)
{
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

int
verify_any(char *stanza_text)
{
    depth = 0;
    if (curr_stanza) {
        stanza_free(curr_stanza);
    }
    XML_Parser parser = XML_ParserCreate(NULL);
    XML_SetElementHandler(parser, start_element, end_element);
    XML_SetCharacterDataHandler(parser, handle_data);

    XML_Parse(parser, stanza_text, strlen(stanza_text), 0);
    XML_ParserFree(parser);

    int result = 0;
    if (timeoutsecs <= 0) {
        result = stanza_verify_any(curr_stanza);
    } else {
        double elapsed = 0.0;
        GTimer *timer = g_timer_new();
        while (elapsed < timeoutsecs * 1.0) {
            result = stanza_verify_any(curr_stanza);
            if (result) {
                break;
            }

            usleep(1000 * 50);
            elapsed = g_timer_elapsed(timer, NULL);
        }
    }

    if (result) {
        log_println("VERIFY SUCCESS: %s", stanza_text);
    } else {
        log_println("VERIFY FAIL: %s", stanza_text);
    }

    return result;
}

int
verify_last(char *stanza_text)
{
    depth = 0;
    if (curr_stanza) {
        stanza_free(curr_stanza);
    }
    XML_Parser parser = XML_ParserCreate(NULL);
    XML_SetElementHandler(parser, start_element, end_element);
    XML_SetCharacterDataHandler(parser, handle_data);

    XML_Parse(parser, stanza_text, strlen(stanza_text), 0);
    XML_ParserFree(parser);

    int result = 0;
    if (timeoutsecs <= 0) {
        result = stanza_verify_last(curr_stanza);
    } else {
        double elapsed = 0.0;
        GTimer *timer = g_timer_new();
        while (elapsed < timeoutsecs * 1.0) {
            result = stanza_verify_last(curr_stanza);
            if (result) {
                break;
            }

            usleep(1000 * 50);
            elapsed = g_timer_elapsed(timer, NULL);
        }
    }

    if (result) {
        log_println("VERIFY LAST SUCCESS: %s", stanza_text);
    } else {
        log_println("VERIFY LAST FAIL: %s", stanza_text);
    }

    return result;
}
