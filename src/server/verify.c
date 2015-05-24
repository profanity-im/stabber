#include <string.h>

#include <expat.h>

#include "server/stanza.h"
#include "server/log.h"

static int depth = 0;
static XMPPStanza *curr_stanza = NULL;

static void
start_element(void *data, const char *element, const char **attributes)
{
    XMPPStanza *stanza = stanza_new(element, attributes);

    if (depth == 0) {
        curr_stanza = stanza;
        curr_stanza->parent = NULL;
    } else {
        stanza->parent = curr_stanza;
        curr_stanza = stanza;
    }

    depth++;
}

static void
end_element(void *data, const char *element)
{
    depth--;

    if (depth > 0) {
        stanza_add_child(curr_stanza->parent, curr_stanza);
        curr_stanza = curr_stanza->parent;
    }
}

static void
handle_data(void *data, const char *content, int length)
{
    if (!curr_stanza->content) {
        curr_stanza->content = g_string_new("");
    }

    g_string_append_len(curr_stanza->content, content, length);
}

int
verify_any(char *stanza_text)
{
    depth = 0;
    if (curr_stanza) {
        stanza_free(curr_stanza);
    }
    XML_Parser parser = XML_ParserCreate(NULL);
    XML_SetElementHandler(parser, start_element, end_element);
    XML_SetCharacterDataHandler(parser, handle_data);

    XML_Parse(parser, stanza_text, strlen(stanza_text), 0);
    XML_ParserFree(parser);

    int result = stanza_verify_any(curr_stanza);
    if (result) {
        log_println("VERIFY SUCCESS: %s", stanza_text);
    } else {
        log_println("VERIFY FAIL: %s", stanza_text);
    }

    return result;
}

int
verify_last(char *stanza_text)
{
    depth = 0;
    if (curr_stanza) {
        stanza_free(curr_stanza);
    }
    XML_Parser parser = XML_ParserCreate(NULL);
    XML_SetElementHandler(parser, start_element, end_element);
    XML_SetCharacterDataHandler(parser, handle_data);

    XML_Parse(parser, stanza_text, strlen(stanza_text), 0);
    XML_ParserFree(parser);

    int result = stanza_verify_last(curr_stanza);
    if (result) {
        log_println("VERIFY LAST SUCCESS: %s", stanza_text);
    } else {
        log_println("VERIFY LAST FAIL: %s", stanza_text);
    }

    return result;
}
