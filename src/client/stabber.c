/*
 * stabber.c
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

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "server/server.h"
#include "server/prime.h"
#include "server/verify.h"

#include "stabber.h"

int
stbbr_start(stbbr_log_t loglevel, int port, int httpport)
{
    return server_run(loglevel, port, httpport);
}

void
stbbr_set_timeout(int seconds)
{
    verify_set_timeout(seconds);
}

int
stbbr_auth_passwd(char *password)
{
    prime_required_passwd(password);
    return 1;
}

int
stbbr_for_id(char *id, char *stream)
{
    prime_for_id(id, stream);
    return 1;
}

int
stbbr_for_query(char *query, char *stream)
{
    prime_for_query(query, stream);
    return 1;
}

void
stbbr_wait_for(char *id)
{
    server_wait_for(id);
}

int
stbbr_last_received(char *stanza)
{
    return verify_last(stanza);
}

int
stbbr_received(char *stanza)
{
    return verify_any(stanza, FALSE);
}

void
stbbr_send(char *stream)
{
    server_send(stream);
}

void
stbbr_stop(void)
{
    server_stop();
}
