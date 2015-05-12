#include <stdio.h>
#include <string.h>
#include <expat.h>
#include <glib.h>

#include "server/parser.h"

static int depth = 0;
static int do_reset = 0;
GString *text = NULL;
static XML_Parser parser;

static void
start_element(void *data, const char *element, const char **attribute)
{
    int i;

    printf("LEVEL: %d\n", depth);
    printf("ELEM: %s\n", element);

    for (i = 0; attribute[i]; i += 2) {
        printf(" ATTR: %s='%s'\n", attribute[i], attribute[i + 1]);
    }

    depth++;
}

static void
end_element(void *data, const char *element)
{
    if (text) {
        printf(" TEXT: %s\n", text->str);
        g_string_free(text, TRUE);
        text = NULL;
    }

    depth--;
    if (depth == 0) {
        do_reset = 1;
        printf("\n");
    }
}

static void
handle_data(void *data, const char *content, int length)
{
    if (!text) {
        text = g_string_new("");
    }

    g_string_append_len(text, content, length);
}

void
parser_init(void)
{
    parser = XML_ParserCreate(NULL);
    XML_SetElementHandler(parser, start_element, end_element);
    XML_SetCharacterDataHandler(parser, handle_data);
}

int
parser_feed(char *chunk, int len)
{
    return XML_Parse(parser, chunk, len, 0);
}

void
parser_close(void)
{
    XML_ParserFree(parser);
}

void
parser_reset(void)
{
    if (do_reset == 1) {
        parser_close();
        parser_init();
        do_reset = 0;
    }
}
