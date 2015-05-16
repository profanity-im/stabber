#ifndef __H_PARSER
#define __H_PARSER

#include "server/stanza.h"
#include "server/xmppclient.h"

typedef void (*stream_start_func)(void);
typedef void (*auth_func)(XMPPStanza *stanza);
typedef void (*id_func)(const char *id);

void parser_init(stream_start_func startcb, auth_func authcb, id_func);
int parser_feed(char *chunk, int len);
void parser_close(void);
void parser_reset(void);
void parser_show_stanzas(void);

#endif
