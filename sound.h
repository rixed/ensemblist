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
#ifndef SOUND_H
#define SOUND_H

extern unsigned nb_musics;
extern unsigned nb_samples;

/* takes 0 if no sounds, 1 if sounds are on */
extern void sound_init(int);
extern void sound_end();
extern void sound_music_start(unsigned);
extern void sound_music_stop();
extern void sound_update();
/* index of sample, volume (0-255), freq (1=normal), pan (0-255) */
extern void sound_sample_start(unsigned, unsigned, float, unsigned);
	
#endif
