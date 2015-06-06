/*
 * stanza.h
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
XMPPStanza* parse_stanza(char *stanza_text);
char* stanza_to_string(XMPPStanza *stanza);

#endif
