#include <stdlib.h>
#include <glib.h>

#include <string.h>

static char *required_passwd = NULL;
static GHashTable *idstubs;

void
prime_required_passwd(char *password)
{
    required_passwd = strdup(password);
}

char *
prime_get_passwd(void)
{
    return required_passwd;
}

void
prime_for(char *id, char *stream)
{
    if (!idstubs) {
        idstubs = g_hash_table_new_full(g_str_hash, g_str_equal, free, free);
    }

    g_hash_table_insert(idstubs, strdup(id), strdup(stream));
}

char*
prime_get_for(const char *id)
{
    if (idstubs) {
        return g_hash_table_lookup(idstubs, id);
    }

    return NULL;
}
