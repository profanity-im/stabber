#ifndef __H_PARSER
#define __H_PARSER

void parser_init(void);
int parser_feed(char *chunk, int len);
void parser_close(void);
void parser_reset(void);

#endif
