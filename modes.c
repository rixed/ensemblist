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
#include "log.h"
#include "modes.h"
/* callbacks for every game mode */

/*
 * The events are treated depending on the current 'runlevel', or 'mode'.
 * A mode is a state of the program, identifying the way events are handled,
 * and corresponding to the location of the gamer in the programm.
 * Initial mode is mode MODE_INIT. Those are defined in modes.h
 * Here is a short description for every mode, followed by the prototypes for events handlers
 * (events handlers are called if not NULL).
 */

/* MODE_INIT
 * Initial state.
 * Used to initialize in opengl what needs to be initialized once and for all when the opengl machine is running
 */

unsigned enigm_to_play = ~0;	/* this must be initialised before switching to play mode */

/* display functions */
extern void m0_display(void);
extern void m1_display(void);
extern void intro_display(void);
extern void play_display(void);
/* mouse clic functions */
extern void intro_clic(int,int);
extern void play_clic(int,int);
/* mouse motion functions */
//extern void intro_motion(float x, float y);
extern void play_motion(float, float);
/* keyboard functions */
extern void intro_key(int, float, float, int);


int mode = MODE_INIT;

void (*mode_display[])(void) = { m0_display, m1_display, intro_display, play_display };
void (*mode_mouse[])(int, int) = { NULL, NULL, intro_clic, play_clic };
void (*mode_mouse_motion[])(float, float) = { NULL, NULL, NULL, play_motion };
void (*mode_key[])(int, float, float, int) = { NULL, NULL, intro_key, NULL };

void mode_change(unsigned new_mode) {
	gltv_log_warning(GLTV_LOG_DEBUG, "Switching to mode %d", new_mode);
	if (MODE_SHUTDOWN==new_mode) {	/* shutdown */
		exit(0);
	}
	mode = new_mode;
}

