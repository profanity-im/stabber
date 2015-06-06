/*
 * stanzas.h
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

#ifndef __H_STANZAS
#define __H_STANZAS

#include <glib.h>

#include "server/stanza.h"

void stanza_add(XMPPStanza *stanza);

int stanza_verify_any(XMPPStanza *stanza);
int stanza_verify_last(XMPPStanza *stanza);

int stanzas_contains_id(char *id);

void stanza_free(XMPPStanza *stanza);
void stanza_free_all(void);

#endif
