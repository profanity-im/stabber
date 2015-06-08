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
#include "server/log.h"

#include "stabber.h"

int
stbbr_start(stbbr_log_t loglevel, int port, int httpport)
{
    return server_run(loglevel, port, httpport);
}

void
stbbr_set_timeout(int seconds)
{
    log_println(STBBR_LOGDEBUG, "Setting timeout: %d seconds", seconds);
    verify_set_timeout(seconds);
}

int
stbbr_auth_passwd(char *password)
{
    log_println(STBBR_LOGDEBUG, "Setting auth password: %s", password);
    prime_required_passwd(password);
    return 1;
}

int
stbbr_for_id(char *id, char *stream)
{
    log_println(STBBR_LOGDEBUG, "Stubbing for id: %s, stanza: %s", id, stream);
    prime_for_id(id, stream);
    return 1;
}

int
stbbr_for_query(char *query, char *stream)
{
    log_println(STBBR_LOGDEBUG, "Stubbing for query: %s, stanza: %s", query, stream);
    prime_for_query(query, stream);
    return 1;
}

void
stbbr_wait_for(char *id)
{
    log_println(STBBR_LOGDEBUG, "Waiting for id: %s", id);
    server_wait_for(id);
}

int
stbbr_last_received(char *stanza)
{
    log_println(STBBR_LOGDEBUG, "verifying last: %s", stanza);
    return verify_last(stanza);
}

int
stbbr_received(char *stanza)
{
    log_println(STBBR_LOGDEBUG, "verifying: %s", stanza);
    return verify_any(stanza);
}

void
stbbr_send(char *stream)
{
    log_println(STBBR_LOGDEBUG, "Sending: %s", stream);
    server_send(stream);
}

void
stbbr_stop(void)
{
    log_println(STBBR_LOGDEBUG, "Stopping stabber");
    server_stop();
}
