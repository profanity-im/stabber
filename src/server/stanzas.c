/*
 * stanzas.c
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
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <fnmatch.h>

#include "server/stanza.h"
#include "server/log.h"

pthread_mutex_t stanzas_lock;
static GList *stanzas;

static gboolean stanzas_initialized = FALSE;

void
stanzas_init(void)
{
    if (stanzas_initialized) {
        return;
    }
    pthread_mutex_init(&stanzas_lock, NULL);
    stanzas_initialized = TRUE;
}

static int _xmpp_attr_equal(XMPPAttr *attr1, XMPPAttr *attr2);
static int _stanzas_equal(XMPPStanza *first, XMPPStanza *second);

int
stanzas_contains_id(char *id)
{
    stanzas_init();
    
    char *name_filter = NULL;
    char *id_filter = id;
    char *colon = strchr(id, ':');
    if (colon) {
        name_filter = g_strndup(id, colon - id);
        id_filter = colon + 1;
    }

    pthread_mutex_lock(&stanzas_lock);
    GList *curr = stanzas;
    while (curr) {
        XMPPStanza *stanza = curr->data;
        
        gboolean name_match = TRUE;
        if (name_filter && g_strcmp0(stanza->name, name_filter) != 0) {
            name_match = FALSE;
        }

        if (name_match) {
            GList *curr_attr = stanza->attrs;
            while (curr_attr) {
                XMPPAttr *attr = curr_attr->data;
                if (g_strcmp0(attr->name, "id") == 0 && fnmatch(id_filter, attr->value, 0) == 0) {
                    pthread_mutex_unlock(&stanzas_lock);
                    g_free(name_filter);
                    return 1;
                }
                curr_attr = g_list_next(curr_attr);
            }
            // If we are looking for ANY id in a specific stanza name
            if (g_strcmp0(id_filter, "*") == 0) {
                pthread_mutex_unlock(&stanzas_lock);
                g_free(name_filter);
                return 1;
            }
        }
        curr = g_list_next(curr);
    }
    pthread_mutex_unlock(&stanzas_lock);
    g_free(name_filter);

    return 0;
}

void
stanzas_add(XMPPStanza *stanza)
{
    stanzas_init();
    pthread_mutex_lock(&stanzas_lock);
    stanzas = g_list_append(stanzas, stanza);
    pthread_mutex_unlock(&stanzas_lock);
}

int
stanzas_verify_any(XMPPStanza *stanza)
{
    stanzas_init();
    pthread_mutex_lock(&stanzas_lock);
    if (!stanzas) {
        pthread_mutex_unlock(&stanzas_lock);
        return 0;
    }

    GList *curr = g_list_last(stanzas);
    while (curr) {
        XMPPStanza *curr_stanza = curr->data;
        if (_stanzas_equal(stanza, curr_stanza) == 0) {
            pthread_mutex_unlock(&stanzas_lock);
            return 1;
        }

        curr = g_list_previous(curr);
    }

    pthread_mutex_unlock(&stanzas_lock);
    return 0;
}

int
stanzas_verify_last(XMPPStanza *stanza)
{
    stanzas_init();
    pthread_mutex_lock(&stanzas_lock);
    if (!stanzas) {
        pthread_mutex_unlock(&stanzas_lock);
        return 0;
    }

    GList *last = g_list_last(stanzas);
    if (!last) {
        pthread_mutex_unlock(&stanzas_lock);
        return 0;
    }

    XMPPStanza *last_stanza = (XMPPStanza *)last->data;
    int res = _stanzas_equal(stanza, last_stanza);
    pthread_mutex_unlock(&stanzas_lock);
    if (res == 0) {
        return 1;
    } else {
        return 0;
    }
}

void
stanzas_free_all(void)
{
    stanzas_init();
    pthread_mutex_lock(&stanzas_lock);
    g_list_free_full(stanzas, (GDestroyNotify)stanza_free);
    stanzas = NULL;
    pthread_mutex_unlock(&stanzas_lock);
}

static int
_xmpp_attr_equal(XMPPAttr *attr1, XMPPAttr *attr2)
{
    if (g_strcmp0(attr1->name, attr2->name) != 0) {
        return -1;
    }

    if (g_strcmp0(attr1->value, "*") == 0) {
        return 0;
    }
    if (g_strcmp0(attr2->value, "*") == 0) {
        return 0;
    }

    if (g_strcmp0(attr1->value, attr2->value) != 0) {
        return -1;
    }

    return 0;
}

static int
_stanzas_equal(XMPPStanza *first, XMPPStanza *second)
{
    // check name
    if (g_strcmp0(first->name, second->name) != 0) {
        return -1;
    }

    // check attribute count
    if (g_list_length(first->attrs) != g_list_length(second->attrs)) {
        return -1;
    }

    // check children count
    if (g_list_length(first->children) != g_list_length(second->children)) {
        return -1;
    }

    // check presence of content
    if (!first->content && second->content) {
        return -1;
    }
    if (first->content && !second->content) {
        return -1;
    }

    // check content is exists
    if (first->content) {
        if (g_strcmp0(first->content->str, second->content->str) != 0) {
            return -1;
        }
    }

    // check attributes
    if (first->attrs) {
        GList *first_curr_attr = first->attrs;
        while (first_curr_attr) {
            XMPPAttr *first_attr = (XMPPAttr *)first_curr_attr->data;

            GList *second_found_attr = g_list_find_custom(second->attrs, first_attr, (GCompareFunc)_xmpp_attr_equal);
            if (!second_found_attr) {
                return -1;
            }

            first_curr_attr = g_list_next(first_curr_attr);
        }
    }

    // check children
    if (first->children) {
        GList *first_curr_child = first->children;
        while (first_curr_child) {
            XMPPStanza *first_child = (XMPPStanza *)first_curr_child->data;
            GList *second_found_child = g_list_find_custom(second->children, first_child, (GCompareFunc)_stanzas_equal);
            if (!second_found_child) {
                return -1;
            }

            first_curr_child = g_list_next(first_curr_child);
        }
    }

    return 0;
}
