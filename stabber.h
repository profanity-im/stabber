/*
 * stabber.h
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

#ifndef __H_STABBER
#define __H_STABBER

int stbbr_start(int port, int httpport);
void stbbr_stop(void);

void stbbr_set_timeout(int seconds);

int stbbr_auth_passwd(char *password);
void stbbr_for_id(char *id, char *stream);

void stbbr_wait_for(char *id);

int stbbr_received(char *stanza);
int stbbr_last_received(char *stanza);

void stbbr_send(char *stream);

#endif
