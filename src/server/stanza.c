#include <stdio.h>
#include <glib.h>

#include "server/stanza.h"

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
