/*
Copyright (C) 2003 Cedric Cellier, Dominique Lavault

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "user.h"
#include "system.h"
#include "memspool.h"
#include "log.h"

/* TODO : sauver/lire aussi les enigmes réussies */

static char *user_file;

char *user_name;
unsigned user_score;

void init_void_user()
{
	user_score = 1;
}

void user_save()
{
	FILE *file;
	file = fopen(user_file, "w+");
	if (!file) {
		gltv_log_warning(GLTV_LOG_MUSTSEE, "user_save: Cannot open file '%s' : %s", user_file, strerror(errno));
	} else {
		if (fwrite(&user_score, sizeof(user_score), 1, file)<1) {
			gltv_log_warning(GLTV_LOG_MUSTSEE, "user_save: Cannot write file '%s' : %s", user_file, strerror(errno));
		}
	}
	gltv_memspool_unregister(user_file);
}

void user_read()
{
	char *user_dir;
	FILE *file;
	const char *filename = "/.ensemblist";
	unsigned u, v;
	if (!sys_get_user_name(&user_name)) {
		gltv_log_warning(GLTV_LOG_MUSTSEE, "user_read: Cannot get user name");
		user_name = "unknown";
	}
	gltv_log_warning(GLTV_LOG_DEBUG, "USER = %s",user_name);
	if (!sys_get_user_dir(&user_dir)) {
		gltv_log_warning(GLTV_LOG_MUSTSEE, "user_read: Cannot get user's directory");
		init_void_user();
		return;
	}
	atexit(user_save);
	u = strlen(user_dir);
	v = strlen(filename);
	user_file = gltv_memspool_alloc(u+v+1);
	if (!user_file) gltv_log_fatal("user_read: Cannot get memory\n");
	memcpy(user_file, user_dir, u);
	memcpy(user_file+u, filename, v+1);
	file = fopen(user_file, "r");
	if (!file) {
		init_void_user();
		return;
	}
	if (fread(&user_score, sizeof(user_score), 1, file)<1) {
		gltv_log_warning(GLTV_LOG_MUSTSEE, "user_read: Cannot read file '%s' : %s", user_file, strerror(errno));
		init_void_user();
		return;
	}
	if (user_score==0) user_score = 1;
}


