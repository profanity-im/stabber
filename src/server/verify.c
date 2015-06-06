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

#include <string.h>
#include <unistd.h>

#include <expat.h>

#include "server/stanza.h"
#include "server/stanzas.h"
#include "server/log.h"

static int timeoutsecs = 0;

void
verify_set_timeout(int seconds)
{
    if (seconds <= 0) {
        timeoutsecs = 0;
    } else {
        timeoutsecs = seconds;
    }
}

int
verify_any(char *stanza_text)
{
    XMPPStanza *stanza = parse_stanza(stanza_text);

    int result = 0;
    if (timeoutsecs <= 0) {
        result = stanzas_verify_any(stanza);
    } else {
        double elapsed = 0.0;
        GTimer *timer = g_timer_new();
        while (elapsed < timeoutsecs * 1.0) {
            result = stanzas_verify_any(stanza);
            if (result) {
                break;
            }

            usleep(1000 * 50);
            elapsed = g_timer_elapsed(timer, NULL);
        }
    }

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
    XMPPStanza *stanza = parse_stanza(stanza_text);

    int result = 0;
    if (timeoutsecs <= 0) {
        result = stanzas_verify_last(stanza);
    } else {
        double elapsed = 0.0;
        GTimer *timer = g_timer_new();
        while (elapsed < timeoutsecs * 1.0) {
            result = stanzas_verify_last(stanza);
            if (result) {
                break;
            }

            usleep(1000 * 50);
            elapsed = g_timer_elapsed(timer, NULL);
        }
    }

    if (result) {
        log_println("VERIFY LAST SUCCESS: %s", stanza_text);
    } else {
        log_println("VERIFY LAST FAIL: %s", stanza_text);
    }

    return result;
}
