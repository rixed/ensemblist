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
#ifndef _GLTV_LOG_
#define _GLTV_LOG_

/* must be called at first. */

typedef enum {
	GLTV_LOG_MUSTSEE=0,
	GLTV_LOG_IMPORTANT,
	GLTV_LOG_OPTIONAL,
	GLTV_LOG_DEBUG
} GLTV_WARN_LEVEL;

extern void gltv_log_init(GLTV_WARN_LEVEL);
/* must be called at the end */
extern void gltv_log_end();
/* for a fatal error (exit with error code = 1) */
extern void gltv_log_fatal(const char *, ...);
/* output a warning */
extern void gltv_log_warning(GLTV_WARN_LEVEL, const char *, ...);

#endif
