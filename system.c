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
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
//#include <assert.h>
#ifndef _WINDOWS
	#include <unistd.h>
	#include <sys/stat.h>
	#include <sys/types.h>
	#include <fcntl.h>
#else
	#include <windows.h>
	#include <direct.h>
#endif

#include "system.h"
#include "memspool.h"
#include "log.h"

#ifdef _WINDOWS
#define GETUSERNAME_MAXLEN 255
char g_login_name[GETUSERNAME_MAXLEN+1];
#endif

int sys_goto_dir(const char *dir)
{
	int ret = chdir(dir);
	if (!ret) return 1;
	else {
		perror("chdir");
		return 0;
	}
}

int sys_make_dir(const char *dir)
{
#ifdef _WINDOWS
	/* TODO */
	return 0;
#else
	int ret;
	struct stat stat_buf;
	if (0==stat(dir, &stat_buf) && S_ISDIR(stat_buf.st_mode)) return 1;	/* the directory already exists */
	ret = mkdir(dir, 0700);
	if (!ret) return 0;
	else {
		perror("mkdir");
		return 0;
	}
#endif
}

int sys_get_user_name(char **dest)
{
#ifdef _WINDOWS
	DWORD dwLen=GETUSERNAME_MAXLEN;
	*dest=g_login_name;
	return (GetUserName(g_login_name, &dwLen)==NULL?0:1);
#else
	char *login_name = getenv("LOGNAME");
	if (!login_name) return 0;
	*dest = login_name;
	return 1;
#endif
}

int sys_get_user_dir(char **dest)
{
#ifdef _WINDOWS
	*dest =".";
	return 1;
#else
	char *user_dir = getenv("HOME");
	if (!user_dir) return 0;
	*dest = user_dir;
	return 1;
#endif
}

/* NOT USED ANYMORE

SYS_MUTEX sys_mutex_new()
{
	pthread_mutex_t *mutex = gltv_memspool_alloc(sizeof(*mutex));
	pthread_mutex_init(mutex, NULL);
	return mutex;
}

void sys_mutex_del(SYS_MUTEX mutex)
{
	pthread_mutex_destroy(mutex);
	gltv_memspool_unregister(mutex);
}

int sys_mutex_blockget(SYS_MUTEX mutex)
{
	int ret = pthread_mutex_lock(mutex);
	if (ret) gltv_log_warning(GLTV_LOG_MUSTSEE, "sys_mutex_blockget error : %s", strerror(ret));
	return ret==0;
}

int sys_mutex_nonblockget(SYS_MUTEX mutex)
{
	int ret = pthread_mutex_trylock(mutex);
	if (ret && ret!=EBUSY) gltv_log_warning(GLTV_LOG_MUSTSEE, "sys_mutex_nonblockget error : %s", strerror(ret));
	return ret==0;
}

int sys_mutex_release(SYS_MUTEX mutex)
{
	int ret = pthread_mutex_unlock(mutex);
	if (ret) gltv_log_warning(GLTV_LOG_MUSTSEE, "sys_mutex_release error : %s", strerror(ret));
	return ret==0;
}
*/
