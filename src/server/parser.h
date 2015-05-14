#ifndef __H_PARSER
#define __H_PARSER

#include "server/xmppclient.h"

typedef void (*stream_start_func)(void * const userdata);
typedef void (*auth_func)(void * const userdata);

void parser_init(XMPPClient *client, stream_start_func startcb, auth_func authcb);
int parser_feed(char *chunk, int len);
void parser_close(void);
void parser_reset(void);
void parser_show_stanzas(void);

#endif
