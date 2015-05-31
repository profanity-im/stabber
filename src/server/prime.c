/*
 * prime.c
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
    required_passwd = NULL;

    if (idstubs) {
        g_hash_table_destroy(idstubs);
    }
    idstubs = NULL;
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
