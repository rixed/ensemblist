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
#ifndef SYSTEM_H
#define SYSTEM_H

#ifdef _WINDOWS
// ¤Krystal¤ windows function name differs
#define strncasecmp _strnicmp
#endif

/* everything that wrapp OS specific functions */

extern int sys_goto_dir(const char *);
extern int sys_get_user_name(char **);
extern int sys_get_user_dir(char **);

/* NOT USED ANYMORE

#ifdef _WINDOWS
#define pthread_mutex_t cacaboudin
#else
#include <pthread.h>
#endif

typedef pthread_mutex_t* SYS_MUTEX;

extern SYS_MUTEX sys_mutex_new();
extern void sys_mutex_del(SYS_MUTEX);
extern int sys_mutex_blockget(SYS_MUTEX);
extern int sys_mutex_nonblockget(SYS_MUTEX);
extern int sys_mutex_release(SYS_MUTEX);
*/


#endif
