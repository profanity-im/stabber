/*
 * stream_parser.h
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

#ifndef __H_STREAM_PARSER
#define __H_STREAM_PARSER

#include "server/stanza.h"

typedef void (*stream_start_func)(void);
typedef void (*auth_func)(XMPPStanza *stanza);
typedef void (*id_func)(const char *id);
typedef void (*query_func)(const char *query, const char *id);

void parser_init(stream_start_func startcb, auth_func authcb, id_func idcb, query_func querycb);
int parser_feed(char *chunk, int len);
void parser_close(void);
void parser_reset(void);
void parser_show_stanzas(void);

#endif
