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
#ifndef MODES_H
#define MODES_H

#define MODE_INIT 0
#define MODE_TESTCSG 1
#define MODE_INTRO 2
#define MODE_PLAY 3	/* init enigm_to_play before switching to this one */
#define MODE_SHUTDOWN 666

extern int mode;

extern unsigned enigm_to_play;
extern int edit_session;
extern char *edit_session_name;
/* change current mode to another */
extern void mode_change(unsigned);

extern void (*mode_display[])();
extern void (*mode_mouse[])(int, int);
extern void (*mode_mouse_motion[])(float, float);
extern void (*mode_key[])(int, float, float, int);

#endif
