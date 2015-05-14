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

void
show_stanza(XMPPStanza *stanza);

#endif
