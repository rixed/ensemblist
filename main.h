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
#ifndef MAIN_H
#define MAIN_H

extern int glut_fenLong;
extern int glut_fenHaut;
extern int glut_fenPosX;
extern int glut_fenPosY;
extern unsigned glut_interval;
extern float glut_mouse_x;
extern float glut_mouse_y;
extern char glut_mouse_changed;
extern int glut_mouse_button;
extern unsigned long glut_mtime;

extern void convert_pixel_to_percent(int x, int y, float *xx, float *yy);

#define STR(x) #x
#define XSTR(x) STR(x)

#endif
