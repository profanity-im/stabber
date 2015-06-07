/*
 * log.c
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
#include <string.h>
#include <errno.h>
#include <sys/prctl.h>

#include <glib.h>
#include <glib/gstdio.h>

#include "stabber.h"

static FILE *logp;
static stbbr_log_t minlevel;

gchar *
_xdg_get_data_home(void)
{
    gchar *xdg_data_home = getenv("XDG_DATA_HOME");
    if (xdg_data_home)
        g_strstrip(xdg_data_home);

    if (xdg_data_home && (strcmp(xdg_data_home, "") != 0)) {
        return strdup(xdg_data_home);
    } else {
        GString *default_path = g_string_new(getenv("HOME"));
        g_string_append(default_path, "/.local/share");
        gchar *result = strdup(default_path->str);
        g_string_free(default_path, TRUE);

        return result;
    }
}

static gchar *
_get_main_log_file(void)
{
    gchar *xdg_data = _xdg_get_data_home();
    GString *logfile = g_string_new(xdg_data);
    g_string_append(logfile, "/stabber/logs/stabber");
    g_string_append(logfile, ".log");
    gchar *result = strdup(logfile->str);
    free(xdg_data);
    g_string_free(logfile, TRUE);

    return result;
}

gboolean
_create_dir(char *name)
{
    struct stat sb;
    if (stat(name, &sb) != 0) {
        if (errno != ENOENT || mkdir(name, S_IRWXU) != 0) {
            return FALSE;
        }
    } else {
        if ((sb.st_mode & S_IFDIR) != S_IFDIR) {
            return FALSE;
        }
    }

    return TRUE;
}

gboolean
_mkdir_recursive(const char *dir)
{
    int i;
    gboolean result = TRUE;

    for (i = 1; i <= strlen(dir); i++) {
        if (dir[i] == '/' || dir[i] == '\0') {
            gchar *next_dir = g_strndup(dir, i);
            result = _create_dir(next_dir);
            g_free(next_dir);
            if (!result) {
                break;
            }
        }
    }

    return result;
}

void
log_init(stbbr_log_t loglevel)
{
    minlevel = loglevel;
    gchar *xdg_data = _xdg_get_data_home();
    GString *log_dir = g_string_new(xdg_data);
    g_string_append(log_dir, "/stabber/logs");
    _mkdir_recursive(log_dir->str);
    g_string_free(log_dir, TRUE);
    g_free(xdg_data);

    gchar *log_file = _get_main_log_file();
    logp = fopen(log_file, "a");
    g_chmod(log_file, S_IRUSR | S_IWUSR);
    free(log_file);
}

static char*
_levelstr(stbbr_log_t loglevel)
{
    switch (loglevel) {
        case STBBR_LOGERROR: return "ERROR";
        case STBBR_LOGWARN:  return "WARN";
        case STBBR_LOGINFO:  return "INFO";
        case STBBR_LOGDEBUG: return "DEBUG";
        default:             return "";
    }
}

void
log_println(stbbr_log_t loglevel, const char * const msg, ...)
{
    if (loglevel >= minlevel) {
        va_list arg;
        va_start(arg, msg);
        GString *fmt_msg = g_string_new(NULL);
        g_string_vprintf(fmt_msg, msg, arg);
        GTimeZone *tz = g_time_zone_new_local();
        GDateTime *dt = g_date_time_new_now(tz);
        gchar *date_fmt = g_date_time_format(dt, "%d/%m/%Y %H:%M:%S");
        char thr_name[16];
        prctl(PR_GET_NAME, thr_name);
        char *levelstr = _levelstr(loglevel);
        fprintf(logp, "%s: [%s] [%s] %s\n", date_fmt, thr_name, levelstr, fmt_msg->str);
        g_date_time_unref(dt);
        g_time_zone_unref(tz);
        fflush(logp);
        g_free(date_fmt);
        g_string_free(fmt_msg, TRUE);
        va_end(arg);
    }
}

void
log_close(void)
{
    if (logp) {
        fclose(logp);
    }
}
