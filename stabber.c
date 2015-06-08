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

#include <glib.h>
#include <stdlib.h>
#include <stdio.h>
#include <stabber.h>
#include <pthread.h>

#include "stabber.h"

int
main(int argc , char *argv[])
{
    int port = 0;
    int httpport = 0;
    char *loglevelarg = "INFO";
    stbbr_log_t loglevel = STBBR_LOGINFO;

    GOptionEntry entries[] =
    {
        { "port", 'p', 0, G_OPTION_ARG_INT, &port, "Listen port", NULL },
        { "http", 'h', 0, G_OPTION_ARG_INT, &httpport, "HTTP Listen port", NULL },
        { "log",'l', 0, G_OPTION_ARG_STRING, &loglevelarg, "Set logging levels, DEBUG, INFO (default), WARN, ERROR", "LEVEL" },
        { NULL }
    };

    GError *error = NULL;
    GOptionContext *context;

    context = g_option_context_new(NULL);
    g_option_context_add_main_entries(context, entries, NULL);
    if (!g_option_context_parse(context, &argc, &argv, &error)) {
        g_print("%s\n", error->message);
        g_option_context_free(context);
        g_error_free(error);
        return 1;
    }

    g_option_context_free(context);

    if (port == 0) {
        printf("Port must be specified with -p <port>\n");
        return 1;
    }

    if (g_strcmp0(loglevelarg, "INFO") == 0) {
        loglevel = STBBR_LOGINFO;
    } else if (g_strcmp0(loglevelarg, "DEBUG") == 0) {
        loglevel = STBBR_LOGDEBUG;
    } else if (g_strcmp0(loglevelarg, "WARN") == 0) {
        loglevel = STBBR_LOGWARN;
    } else if (g_strcmp0(loglevelarg, "ERROR") == 0) {
        loglevel = STBBR_LOGERROR;
    } else {
        printf("Invalid log level supplied, must be one of DEBUG, INFO, WARN, ERROR.\n");
        return 1;
    }

    stbbr_start(loglevel, port, httpport);

    pthread_exit(0);
}
