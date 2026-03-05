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

#include <string.h>
#include <fnmatch.h>
#include <glib.h>

#include "server/stanzas.h"
#include "server/log.h"

static GList *received_stanzas = NULL;
static pthread_mutex_t stanzas_lock = PTHREAD_MUTEX_INITIALIZER;

void
stanzas_init(void)
{
    pthread_mutex_lock(&stanzas_lock);
    received_stanzas = NULL;
    pthread_mutex_unlock(&stanzas_lock);
}

void
stanzas_free_all(void)
{
    pthread_mutex_lock(&stanzas_lock);
    g_list_free_full(received_stanzas, (GDestroyNotify)stanza_free);
    received_stanzas = NULL;
    pthread_mutex_unlock(&stanzas_lock);
}

void
stanzas_add(XMPPStanza *stanza)
{
    pthread_mutex_lock(&stanzas_lock);
    received_stanzas = g_list_append(received_stanzas, stanza);
    pthread_mutex_unlock(&stanzas_lock);
}

int
stanzas_contains_id(const char *id_filter)
{
    int result = FALSE;
    pthread_mutex_lock(&stanzas_lock);
    GList *curr = received_stanzas;
    while (curr) {
        XMPPStanza *stanza = (XMPPStanza *)curr->data;
        const char *id = stanza_get_id(stanza);
        if (id) {
            gchar *full_id = g_strdup_printf("%s:%s", stanza->name, id);
            if (fnmatch(id_filter, full_id, 0) == 0) {
                result = TRUE;
                g_free(full_id);
                break;
            }
            g_free(full_id);
        }
        curr = g_list_next(curr);
    }
    pthread_mutex_unlock(&stanzas_lock);

    return result;
}

static int
_xmpp_attr_equal(XMPPAttr *received_attr, XMPPAttr *template_attr)
{
    if (g_strcmp0(template_attr->name, received_attr->name) != 0) {
        return -1;
    }

    if (fnmatch(template_attr->value, received_attr->value, 0) == 0) {
        return 0;
    }

    return -1;
}

static int
_stanzas_equal(XMPPStanza *received, XMPPStanza *template)
{
    // Check name
    if (g_strcmp0(template->name, "*") != 0 && g_strcmp0(template->name, received->name) != 0) {
        return -1;
    }

    // Check content (if template specifies content)
    if (template->content && (template->content->len > 0)) {
        if (!received->content || g_strcmp0(template->content->str, received->content->str) != 0) {
            // Check if template content is a wildcard
            if (g_strcmp0(template->content->str, "*") != 0) {
                return -1;
            }
        }
    }

    // Check attributes: every attribute in template MUST exist in received
    if (template->attrs) {
        GList *curr_template_attr = template->attrs;
        while (curr_template_attr) {
            XMPPAttr *t_attr = (XMPPAttr *)curr_template_attr->data;
            
            GList *found_received_attr = g_list_find_custom(received->attrs, t_attr, (GCompareFunc)_xmpp_attr_equal);
            if (!found_received_attr) {
                return -1;
            }

            curr_template_attr = g_list_next(curr_template_attr);
        }
    }

    // Every child in template MUST exist in received
    if (template->children) {
        GList *curr_template_child = template->children;
        while (curr_template_child) {
            XMPPStanza *t_child = (XMPPStanza *)curr_template_child->data;
            
            // Look for this child in received
            GList *found_received_child = g_list_find_custom(received->children, t_child, (GCompareFunc)_stanzas_equal);
            if (!found_received_child) {
                return -1;
            }

            curr_template_child = g_list_next(curr_template_child);
        }
    }

    return 0;
}

int
stanzas_verify_any(XMPPStanza *template, int clear)
{
    int result = FALSE;
    pthread_mutex_lock(&stanzas_lock);
    GList *curr = received_stanzas;
    while (curr) {
        XMPPStanza *received = (XMPPStanza *)curr->data;
        if (_stanzas_equal(received, template) == 0) {
            result = TRUE;
            if (clear) {
                received_stanzas = g_list_delete_link(received_stanzas, curr);
                stanza_free(received);
            }
            break;
        }
        curr = g_list_next(curr);
    }
    pthread_mutex_unlock(&stanzas_lock);

    return result;
}

int
stanzas_verify_last(XMPPStanza *template)
{
    int result = FALSE;
    pthread_mutex_lock(&stanzas_lock);
    GList *last = g_list_last(received_stanzas);
    if (last) {
        XMPPStanza *received = (XMPPStanza *)last->data;
        if (_stanzas_equal(received, template) == 0) {
            result = TRUE;
        }
    }
    pthread_mutex_unlock(&stanzas_lock);

    return result;
}
