/*
 * verify.c
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
#include <unistd.h>

#include "stabber.h"
#include "server/stanzas.h"
#include "server/log.h"

static int verify_timeout = 10;

void
verify_set_timeout(int seconds)
{
    verify_timeout = seconds;
}

int
verify_any(char *stanza_text, int clear)
{
    XMPPStanza *stanza = stanza_parse(stanza_text);
    GTimer *timer = g_timer_new();
    int result = FALSE;

    while (g_timer_elapsed(timer, NULL) < verify_timeout) {
        result = stanzas_verify_any(stanza, clear);
        if (result) {
            break;
        }
        usleep(1000 * 10);
    }

    g_timer_destroy(timer);
    stanza_free(stanza);

    if (result) {
        log_println(STBBR_LOGINFO, "VERIFY ANY SUCCESS: %s", stanza_text);
    } else {
        log_println(STBBR_LOGINFO, "VERIFY ANY FAIL: %s", stanza_text);
    }

    return result;
}

int
verify_last(char *stanza_text)
{
    XMPPStanza *stanza = stanza_parse(stanza_text);
    GTimer *timer = g_timer_new();
    int result = FALSE;

    while (g_timer_elapsed(timer, NULL) < verify_timeout) {
        result = stanzas_verify_last(stanza);
        if (result) {
            break;
        }
        usleep(1000 * 10);
    }

    g_timer_destroy(timer);
    stanza_free(stanza);

    if (result) {
        log_println(STBBR_LOGINFO, "VERIFY LAST SUCCESS: %s", stanza_text);
    } else {
        log_println(STBBR_LOGINFO, "VERIFY LAST FAIL: %s", stanza_text);
    }

    return result;
}
