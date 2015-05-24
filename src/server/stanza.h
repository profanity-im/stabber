#ifndef __H_STANZA
#define __H_STANZA

#include <glib.h>

typedef struct xmpp_attr_t {
    char *name;
    char *value;
} XMPPAttr;

typedef struct xmpp_stanza_t {
    char *name;
    GList *attrs;
    GList *children;
    GString *content;
    struct xmpp_stanza_t *parent;
} XMPPStanza;

XMPPStanza* stanza_new(const char *name, const char **attributes);
void stanza_show(XMPPStanza *stanza);
void stanza_show_all(void);
void stanza_add(XMPPStanza *stanza);
void stanza_add_child(XMPPStanza *parent, XMPPStanza *child);
const char* stanza_get_id(XMPPStanza *stanza);

XMPPStanza* stanza_get_child_by_ns(XMPPStanza *stanza, char *ns);
XMPPStanza* stanza_get_child_by_name(XMPPStanza *stanza, char *name);

int stanza_verify_any(XMPPStanza *stanza);
int stanza_verify_last(XMPPStanza *stanza);

void stanza_free(XMPPStanza *stanza);
void stanza_free_all(void);

#endif
