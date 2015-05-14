#include <stdio.h>
#include <glib.h>

#include "server/stanza.h"

static GList *stanzas;

void
stanza_show(XMPPStanza *stanza)
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
        printf("\n");
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
