#ifndef __H_STANZA
#define __H_STANZA

typedef struct xmpp_attr_t {
    const char *name;
    const char *value;
} XMPPAttr;

typedef struct xmpp_stanza_t {
    const char *name;
    GList *attrs;
    GList *children;
    GString *content;
    struct xmpp_stanza_t *parent;
} XMPPStanza;

void stanza_show(XMPPStanza *stanza);
void stanza_show_all(void);
void stanza_add(XMPPStanza *stanza);
void stanza_add_child(XMPPStanza *parent, XMPPStanza *child);
XMPPStanza* stanza_get_child_by_ns(XMPPStanza *stanza, char *ns);

#endif
