#include <stdlib.h>
#include <glib.h>

#include <string.h>

static char *required_passwd = NULL;
static GHashTable *idstubs = NULL;

void
prime_init(void)
{
    required_passwd = strdup("password");
    idstubs = g_hash_table_new_full(g_str_hash, g_str_equal, free, free);
}

void
prime_free_all(void)
{
    free(required_passwd);
    if (idstubs) {
        g_hash_table_destroy(idstubs);
    }
}

void
prime_required_passwd(char *password)
{
    free(required_passwd);
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
    if (idstubs) {
        g_hash_table_insert(idstubs, strdup(id), strdup(stream));
    }
}

char*
prime_get_for(const char *id)
{
    return g_hash_table_lookup(idstubs, id);
}
