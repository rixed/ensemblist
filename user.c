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

static char *user_score_file;	/* score file */
char *user_rc_dir;
char *user_name;
unsigned user_score;

void init_void_user()
{
	user_score = 1;
}

void user_save()
{
	FILE *file;
	file = fopen(user_score_file, "w+");
	if (!file) {
		gltv_log_warning(GLTV_LOG_MUSTSEE, "user_save: Cannot open file '%s' : %s", user_score_file, strerror(errno));
	} else {
		if (fwrite(&user_score, sizeof(user_score), 1, file)<1) {
			gltv_log_warning(GLTV_LOG_MUSTSEE, "user_save: Cannot write file '%s' : %s", user_score_file, strerror(errno));
		}
	}
	gltv_memspool_unregister(user_score_file);
}

void user_read()
{
	char *user_dir;
	const char *config_dir =
#ifdef _WINDOWS
		"\\.ensemblist";
#else
		"/.ensemblist";
#endif
	FILE *file;
	const char *score_file =
#ifdef _WINDOWS
		"\\score";
#else
		"/score";
#endif
	unsigned u, v, w;
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
	u = strlen(user_dir);
	v = strlen(config_dir);
	w = strlen(score_file);
	user_rc_dir = gltv_memspool_alloc(u+v+1);
	user_score_file = gltv_memspool_alloc(u+v+w+1);
	if (!user_rc_dir || !user_score_file) gltv_log_fatal("user_read: Cannot get memory\n");
	memcpy(user_rc_dir, user_dir, u);
	memcpy(user_rc_dir+u, config_dir, v+1);
	if (!sys_make_dir(user_rc_dir)) {
		gltv_log_warning(GLTV_LOG_MUSTSEE, "user_read: Cannot get user's config directory");
		init_void_user();
		return;
	}
	memcpy(user_score_file, user_rc_dir, u+v);
	memcpy(user_score_file+u+v, score_file, w+1);
	file = fopen(user_score_file, "r");
	if (!file) {
		init_void_user();
		return;
	}
	if (fread(&user_score, sizeof(user_score), 1, file)<1) {
		gltv_log_warning(GLTV_LOG_MUSTSEE, "user_read: Cannot read file '%s' : %s", user_score_file, strerror(errno));
		init_void_user();
		return;
	}
	if (user_score==0) user_score = 1;
	atexit(user_save);
}


