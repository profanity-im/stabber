#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "server/stanza.h"
#include "server/log.h"

static GList *stanzas;

XMPPStanza*
stanza_new(const char *name, const char **attributes)
{
    XMPPStanza *stanza = malloc(sizeof(XMPPStanza));
    stanza->name = strdup(name);
    stanza->content = NULL;
    stanza->children = NULL;
    stanza->attrs = NULL;
    if (attributes[0]) {
        int i;
        for (i = 0; attributes[i]; i += 2) {
            XMPPAttr *attr = malloc(sizeof(XMPPAttr));
            attr->name = strdup(attributes[i]);
            attr->value = strdup(attributes[i+1]);
            stanza->attrs = g_list_append(stanza->attrs, attr);
        }
    }

    return stanza;
}

void
stanza_show(XMPPStanza *stanza)
{
    log_println("NAME   : %s", stanza->name);

    if (stanza->content && stanza->content->len > 0) {
        log_println("CONTENT: %s", stanza->content->str);
    }

    if (stanza->attrs) {
        GList *curr_attr = stanza->attrs;
        while (curr_attr) {
            XMPPAttr *attr = curr_attr->data;
            log_println("ATTR   : %s='%s'", attr->name, attr->value);

            curr_attr = g_list_next(curr_attr);
        }
    }

    if (stanza->children) {
        log_println("CHILDREN:");
        GList *curr_child = stanza->children;
        while (curr_child) {
            XMPPStanza *child = curr_child->data;
            stanza_show(child);
            curr_child = g_list_next(curr_child);
        }
    }
}

void
stanza_show_all(void)
{
    GList *curr = stanzas;
    while (curr) {
        XMPPStanza *stanza = curr->data;
        stanza_show(stanza);
        log_println("");
        curr = g_list_next(curr);
    }
}

void
stanza_add(XMPPStanza *stanza)
{
    stanzas = g_list_append(stanzas, stanza);
}

void
stanza_add_child(XMPPStanza *parent, XMPPStanza *child)
{
    parent->children = g_list_append(parent->children, child);
}

XMPPStanza*
stanza_get_child_by_ns(XMPPStanza *stanza, char *ns)
{
    if (!stanza) {
        return NULL;
    }

    if (!stanza->children) {
        return NULL;
    }

    GList *curr_child = stanza->children;
    while (curr_child) {
        XMPPStanza *child = curr_child->data;
        if (!child->attrs) {
            return NULL;
        }
        GList *curr_attr = child->attrs;
        while (curr_attr) {
            XMPPAttr *attr = curr_attr->data;
            if ((g_strcmp0(attr->name, "xmlns") == 0) && (g_strcmp0(attr->value, ns) == 0)) {
                return child;
            }

            curr_attr = g_list_next(curr_attr);
        }

        curr_child = g_list_next(curr_child);
    }

    return NULL;
}

XMPPStanza*
stanza_get_child_by_name(XMPPStanza *stanza, char *name)
{
    if (!stanza) {
        return NULL;
    }

    if (!stanza->children) {
        return NULL;
    }

    GList *curr_child = stanza->children;
    while (curr_child) {
        XMPPStanza *child = curr_child->data;
        if (g_strcmp0(child->name, name) == 0) {
            return child;
        }

        curr_child = g_list_next(curr_child);
    }

    return NULL;
}

const char *
stanza_get_id(XMPPStanza *stanza)
{
    if (!stanza) {
        return NULL;
    }

    if (!stanza->attrs) {
        return NULL;
    }

    GList *curr_attr = stanza->attrs;
    while (curr_attr) {
        XMPPAttr *attr = curr_attr->data;
        if (g_strcmp0(attr->name, "id") == 0) {
            return attr->value;
        }

        curr_attr = g_list_next(curr_attr);
    }

    return NULL;
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

int
stanza_verify_any(XMPPStanza *stanza)
{
    if (!stanzas) {
        return 0;
    }

    GList *curr = g_list_last(stanzas);
    while (curr) {
        XMPPStanza *curr_stanza = curr->data;
        if (_stanzas_equal(stanza, curr_stanza) == 0) {
            return 1;
        }

        curr = g_list_previous(curr);
    }

    return 0;
}

int
stanza_verify_last(XMPPStanza *stanza)
{
    if (!stanzas) {
        return 0;
    }

    GList *last = g_list_last(stanzas);
    if (!last) {
        return 0;
    }

    XMPPStanza *last_stanza = (XMPPStanza *)last->data;

    int res = _stanzas_equal(stanza, last_stanza);
    if (res == 0) {
        return 1;
    } else {
        return 0;
    }
}

static void
_attrs_free(XMPPAttr *attr)
{
    if (attr) {
        free(attr->name);
        free(attr->value);
        free(attr);
    }
}

void
stanza_free(XMPPStanza *stanza)
{
    if (stanza) {
        free(stanza->name);
        if (stanza->content) {
            g_string_free(stanza->content, TRUE);
        }
        g_list_free_full(stanza->attrs, (GDestroyNotify)_attrs_free);
        g_list_free_full(stanza->children, (GDestroyNotify)stanza_free);
        free(stanza);
    }
}

void
stanza_free_all(void)
{
    g_list_free_full(stanzas, (GDestroyNotify)stanza_free);
    stanzas = NULL;
}
