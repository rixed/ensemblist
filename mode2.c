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
#include <math.h>
#include <string.h>
#include <assert.h>
#include "draw.h"
#include "modes.h"
#include "slist.h"
#include "log.h"
#include "memspool.h"
#include "opengl.h"
#include "geom.h"
#include "main.h"
#include "slist.h"
#include "sound.h"
#include "mode2.h"
#include "user.h"

#ifdef _WINDOWS
#define drand48() ((double)rand()/RAND_MAX)
#endif

/* INTRO */

#define NB_CHARS_X 160
#define NB_CHARS_Y 120
#define SIZE_CHAR .4
#define NB_QUADS 40
#define SIZE_QUAD_X (NB_CHARS_X*SIZE_CHAR/NB_QUADS)
#define SIZE_QUAD_Y (NB_CHARS_Y*SIZE_CHAR/NB_QUADS)
#define SIZE_QUAD_TEX_X (8.*SIZE_QUAD_X/SIZE_CHAR)	/* 8 dots per char */
#define SIZE_QUAD_TEX_Y (8.*SIZE_QUAD_Y/SIZE_CHAR)

char quit=0;

static position camera = {
	{
		1,0,0,0,
		0,1,0,0,
		0,0,1,0,
		-7,-15,-.5,1
	},
	NULL,
	0,
	"camera",
	0,
};
static GLfloat target[][16] = {
	{	/* begin */
		1,0,0,0,
		0,.9995,-.01,0,
		0,.01,.9995,0,
		-6.8,-13,-1.3,1
	},
	{	/* quit */
		1,0,0,0,
		0,1,0,0,
		0,0,1,0,
		-20,-11,-.8,1
	},
	{	/* ok */
		1,0,0,0,
		0,.9995,-.01,0,
		0,.01,.9995,0,
		-5,-7,-2.4,1
	},
		{	/* jouer */
		1,0,0,0,
		0,.9995,-.01,0,
		0,.01,.9995,0,
		2.2,+6.5,-1.2,1
	},
		{	/* tournoi */
		1,0,0,0,
		0,.9995,-.01,0,
		0,.01,.9995,0,
		2.2,+13,-2.5,1
	},
		{	/* libre */
		1,0,0,0,
		0,.9995,-.01,0,
		0,.01,.9995,0,
		-13,+13,-2.4,1
	},
	{	/* editer */
		1,0,0,0,
		0,.9995,-.01,0,
		0,.01,.9995,0,
		-7,-3,-2.2,1
	},
	{	/* credits */
		1,0,0,0,
		0,.9995,-.01,0,
		0,.01,.9995,0,
		-9.5,4.5,-1.4,1
	}
};
static unsigned target_id;
static GLfloat projection[16];

static texture *dot=NULL, *dot_fill=NULL;

static unsigned selected_x, selected_y;
static char some_selected = 0;

static unsigned init = 0;

#define FRUSTRUM_Z (-.3)

static void unproj_to_char(double x2d, double y2d, GLfloat *cam, int *tx, int *ty)
{
	double x,y,z;
	double cx,cy,cz;
	double mx,my;
	/* calc CM in (O) */
	x = x2d*cam[0] + y2d*cam[1] + FRUSTRUM_Z*cam[2];
	y = x2d*cam[4] + y2d*cam[5] + FRUSTRUM_Z*cam[6];
	z = x2d*cam[8] + y2d*cam[9] + FRUSTRUM_Z*cam[10];
	/* calc CO in (O) */
	cx = cam[0]*cam[12] + cam[1]*cam[13] + cam[2]*cam[14];
	cy = cam[4]*cam[12] + cam[5]*cam[13] + cam[6]*cam[14];
	cz = cam[8]*cam[12] + cam[9]*cam[13] + cam[10]*cam[14];
	/* calc mx, my in O, projected on the plane */
	mx = cz/z;
	my = mx*y -cy;
	mx = mx*x -cx;
	/* which char is this ? */
	*tx = (int)((mx + (NB_CHARS_X*.5)*SIZE_CHAR)/SIZE_CHAR);
	*ty = (int)(((NB_CHARS_Y*.5)*SIZE_CHAR - my)/SIZE_CHAR);
}

#include "digital.h"
static void put_char(unsigned x, unsigned y, char c, float o_x, float o_y)	/* c is already converted */
{	
	GLfloat px, py, px_init;
	unsigned i,j;
	unsigned char *digit;
	px_init = -(NB_CHARS_X*.5)*SIZE_CHAR + x*(float)SIZE_CHAR + o_x;
	py = NB_CHARS_Y*.5*SIZE_CHAR - y*(float)SIZE_CHAR + o_y;
	assert(c<=47);
	digit = digital_bits+8*(unsigned)c;
	glBegin(GL_QUADS);
	for (j=0; j<8; j++) {
		px = px_init;
		for (i=0; i<8; i++) {
			if ((digit[j]>>i)&1) {
				glTexCoord2f(0,0);
				glNormal3f(0, 0, 1);
				glVertex3f(px, py, 0);
				glTexCoord2f(1,0);
				glNormal3f(0, 0, 1);
				glVertex3f(px+SIZE_CHAR*1./8., py, 0);
				glTexCoord2f(1,1);
				glNormal3f(0, 0, 1);
				glVertex3f(px+SIZE_CHAR*1./8., py-SIZE_CHAR*1./8., 0);
				glTexCoord2f(0,1);
				glNormal3f(0, 0, 1);
				glVertex3f(px, py-SIZE_CHAR*1./8., 0);
			}
			px += SIZE_CHAR*1./8.;
		}
		py -= SIZE_CHAR*1./8.;
	}
	glEnd();
}

#include "menu_digits.h"

static void convert_str(char *str, unsigned len)
{
	while (len>0) {
		if ('a'<=*str && 'z'>=*str) {
			*str -= 'a';
		} else if ('A'<=*str && 'Z'>=*str) {
			*str -= 'A';
		} else if ('0'<=*str && '9'>=*str) {
			*str -= '0';
			*str += 26;
		} else if (' '==*str) {
			*str = 36;
		} else if ('|'==*str) {
			*str = 37;
		} else if ('-'==*str) {
			*str = 38;
		} else if ('+'==*str) {
			*str = 39;
		} else if ('*'==*str) {
			*str = 40;
		} else if ('\\'==*str) {
			*str = 41;
		} else if ('/'==*str) {
			*str = 42;
		} else if ('<'==*str) {
			*str = 43;
		} else if ('>'==*str) {
			*str = 44;
		} else if ('!'==*str) {
			*str = 45;
		} else if (':'==*str) {
			*str = 46;
		} else if ('.'==*str) {
			*str = 47;
		} else {
			gltv_log_warning(GLTV_LOG_OPTIONAL, "not a convertible ASCII : '%c' (%d)", *str, (int)*str);
			*str = 36;
		}
		str++;
		len--;
	}
}

void convert_to_ascii(char *dest, char *str, int len)
{
	while (len>0) {
		if (*str>=0 && *str<26) {
			*dest = 'a'+*str;
		} else if (*str>=26 && *str<36) {
			*dest = '0'+(*str-26);
		} else if (36==*str) {
			*dest = ' ';
		} else if (38==*str) {
			*dest = '-';
		} else if (46==*str) {
			*dest = ':';
		} else if (47==*str) {
			*dest = '.';
		} else {
			*dest = ' ';
		}
		str++;
		dest++;
		len--;
	}
}

static unsigned nb_total_enigms;
static unsigned nb_scored_enigms;
static unsigned nb_free_enigms;
static unsigned first_scored_enigm;
static unsigned offset_scored_enigms;
static unsigned offset_free_enigms;
static unsigned nb_editable_enigms;
static unsigned offset_editable_enigms;

static void update_scored_enigms()
{
	/* first scored enigm lies in string 2.85, and its score at 4.85, and others untill 2/4.94 */
	unsigned i, ii;
	size_t l, ll;
	char s[5];
	int si;
	unsigned score;
	for (ii=0, i=first_scored_enigm+offset_scored_enigms; ii<10; i++, ii++) {
		enigm *e;
		if (i<nb_total_enigms) {
			e = gltv_slist_get(enigms, i);
			l = strlen(e->name);
			ll = l;
			if (lines[85+ii].string[2].len<l) ll=lines[85+ii].string[2].len;
			memcpy(lines[85+ii].string[2].str, e->name, ll);
			memset(lines[85+ii].string[2].str+ll, ' ', lines[85+ii].string[2].len-ll);			
			score = e->score;
			for (si=4; si>=0; si--) {
				s[si] = (score%10)+'0';
				score /= 10;
			}
			assert(lines[85+ii].string[4].len == 5);
			memcpy(lines[85+ii].string[4].str, s, 5);
		} else {
			memset(lines[85+ii].string[2].str, ' ', lines[85+ii].string[2].len);
			memset(lines[85+ii].string[4].str, ' ', lines[85+ii].string[4].len);
		}
		convert_str(lines[85+ii].string[2].str, lines[85+ii].string[2].len);
		convert_str(lines[85+ii].string[4].str, lines[85+ii].string[4].len);
	}
}

static void init_scored_enigms()
{
	unsigned i;
	nb_total_enigms = gltv_slist_size(enigms);
	for (i=0; i<nb_total_enigms; i++) {
		enigm *e = gltv_slist_get(enigms, i);
		if (e->score>0) {
			first_scored_enigm = i;
			break;
		}
	}
	nb_scored_enigms = nb_total_enigms - first_scored_enigm;
	offset_scored_enigms = 0;
	update_scored_enigms();
}

static void run_scored_enigm(unsigned i)
{
	unsigned ei = first_scored_enigm+offset_scored_enigms+i;
	if (ei<nb_total_enigms) {
		enigm *e;
		e = gltv_slist_get(enigms, ei);
		if (e->score>user_score) {
			sound_sample_start(2, 250, 1, 128);
		} else {
			enigm_to_play = ei;
			init = 0;
			edit_session = 0;
			mode_change(MODE_PLAY);
		}
	}
}

void scored_enigms_scroll(int delta)
{
	int n_o = (int)offset_scored_enigms+delta;
	if (n_o<(int)first_scored_enigm) n_o = first_scored_enigm;
	else if (n_o>=(int)nb_total_enigms) n_o = nb_total_enigms-1;
	offset_scored_enigms = n_o;
	update_scored_enigms();
}

static void update_free_enigms()
{
	/* first free enigm lies in string 7.85, and others untill 7.94 */
	unsigned i, ii;
	size_t l, ll;
	for (ii=0, i=offset_free_enigms; ii<10; i++, ii++) {
		enigm *e;
		if (i<nb_free_enigms) {
			e = gltv_slist_get(enigms, i);
			l = strlen(e->name);
			ll = l;
			if (lines[85+ii].string[7].len<l) ll=lines[85+ii].string[7].len;
			memcpy(lines[85+ii].string[7].str, e->name, ll);
			memset(lines[85+ii].string[7].str+ll, ' ', lines[85+ii].string[7].len-ll);			
		} else {
			memset(lines[85+ii].string[7].str, ' ', lines[85+ii].string[7].len);
		}
		convert_str(lines[85+ii].string[7].str, lines[85+ii].string[7].len);
	}
}

static void init_free_enigms()
{
	nb_free_enigms = first_scored_enigm;
	offset_free_enigms = 0;
	update_free_enigms();
}

static void run_free_enigm(unsigned i)
{
	unsigned ei = offset_free_enigms+i;
	if (ei<nb_free_enigms) {
		enigm_to_play = ei;
		init = 0;
		edit_session = 0;
		mode_change(MODE_PLAY);
	}
}

void free_enigms_scroll(int delta)
{
	int n_o = (int)offset_free_enigms+delta;
	if (n_o<0) n_o = 0;
	else if (n_o>=nb_free_enigms) n_o = nb_free_enigms-1;
	offset_free_enigms = n_o;
	update_free_enigms();
}

static void update_editable_enigms()
{
	unsigned i, ii;
	size_t l, ll;
	for (ii=0, i=offset_editable_enigms; ii<6; i++, ii++) {
		enigm *e;
		if (i<nb_editable_enigms) {
			e = gltv_slist_get(editable_enigms, i);
			l = strlen(e->name);
			ll = l;
			if (lines[52+ii].string[3].len<l) ll=lines[52+ii].string[3].len;
			memcpy(lines[52+ii].string[3].str, e->name, ll);
			memset(lines[52+ii].string[3].str+ll, '.', lines[52+ii].string[3].len-ll);
			lines[52+ii].string[5].str[0] = 'x';
			lines[52+ii].string[7].str[0] = 'x';
		} else {
			memset(lines[52+ii].string[3].str, '.', lines[52+ii].string[3].len);
			lines[52+ii].string[5].str[0] = ' ';
			lines[52+ii].string[7].str[0] = ' ';
		}
		convert_str(lines[52+ii].string[3].str, lines[52+ii].string[3].len);
		convert_str(lines[52+ii].string[5].str, 1);
		convert_str(lines[52+ii].string[7].str, 1);
	}
}

static void init_editable_enigms()
{
	nb_editable_enigms = gltv_slist_size(editable_enigms);
	offset_editable_enigms = 0;
	update_editable_enigms();
}

static int cursor_x;
static int cursor_str;
static int cursor_line;

static void cursor_rem()
{
	if (cursor_line) {
		int end = lines[cursor_line].string[cursor_str].len-1;
		memmove(&lines[cursor_line].string[cursor_str].str[cursor_x], &lines[cursor_line].string[cursor_str].str[cursor_x+1], end-cursor_x);
		lines[cursor_line].string[cursor_str].str[end] = '.';
		convert_str(&lines[cursor_line].string[cursor_str].str[end], 1);
		cursor_line = 0;
	}
}

static void cursor_set(int line, int str, int x)
{
	assert(line>0 && line<sizeof(lines));
	assert(str>=0 && str<lines[line].nb_strings);
	assert(x>=0 && x<lines[line].string[str].len);
	cursor_line = line;
	cursor_str = str;
	cursor_x = x;
	memmove(&lines[cursor_line].string[cursor_str].str[cursor_x+1], &lines[cursor_line].string[cursor_str].str[cursor_x], lines[cursor_line].string[cursor_str].len-cursor_x-1);
	lines[cursor_line].string[cursor_str].str[cursor_x] = '|';
	convert_str(&lines[cursor_line].string[cursor_str].str[cursor_x], 1);
}

static void cursor_moveto(int line, int str, int x)
{
	cursor_rem();
	cursor_set(line, str, x);
}

static void cursor_write(unsigned char c)
{
	if (cursor_line && cursor_x<lines[cursor_line].string[cursor_str].len-1) {
		lines[cursor_line].string[cursor_str].str[cursor_x] = c;
		cursor_x++;
		memmove(&lines[cursor_line].string[cursor_str].str[cursor_x+1], &lines[cursor_line].string[cursor_str].str[cursor_x], lines[cursor_line].string[cursor_str].len-cursor_x-1);
		lines[cursor_line].string[cursor_str].str[cursor_x] = '|';
		convert_str(&lines[cursor_line].string[cursor_str].str[cursor_x-1], 2);
	}
}

static void cursor_del_before()
{
	if (cursor_line && cursor_x>0) {
		int end = lines[cursor_line].string[cursor_str].len-1;
		memmove(&lines[cursor_line].string[cursor_str].str[cursor_x-1], &lines[cursor_line].string[cursor_str].str[cursor_x], lines[cursor_line].string[cursor_str].len-cursor_x);
		lines[cursor_line].string[cursor_str].str[end] = '.';
		convert_str(&lines[cursor_line].string[cursor_str].str[end], 1);
		cursor_x--;
	}
}

static void cursor_del_after()
{
	if (cursor_line && cursor_x<lines[cursor_line].string[cursor_str].len-1) {
		int end = lines[cursor_line].string[cursor_str].len-1;
		memmove(&lines[cursor_line].string[cursor_str].str[cursor_x+1], &lines[cursor_line].string[cursor_str].str[cursor_x+2], lines[cursor_line].string[cursor_str].len-cursor_x-2);
		lines[cursor_line].string[cursor_str].str[end] = '.';
		convert_str(&lines[cursor_line].string[cursor_str].str[end], 1);
	}
}

static void cursor_left()
{
	if (cursor_line && cursor_x>0) {
		cursor_moveto(cursor_line, cursor_str, cursor_x-1);
	}
}

static void cursor_right()
{
	if (cursor_line && cursor_x<lines[cursor_line].string[cursor_str].len-1) {
		cursor_moveto(cursor_line, cursor_str, cursor_x+1);
	}
}

void editable_enigms_scroll(int delta)
{
	int n_o = (int)offset_editable_enigms+delta;
	if (n_o<0) n_o = 0;
	else if (n_o>=nb_editable_enigms) n_o = nb_editable_enigms-1;
	cursor_rem();
	offset_editable_enigms = n_o;
	update_editable_enigms();
}

void intro_key(int c, float x, float y, int special)
{
	if (!cursor_line) return;
	if (!special) switch (c) {
		case 8:
			cursor_del_before();
			break;
		case 127:
			cursor_del_after();
			break;
		case 13:
			cursor_rem();
			break;
		default:
			cursor_write(c);
			break;
	} else switch (c) {
		case GLUT_KEY_LEFT:
			cursor_left();
			break;
		case GLUT_KEY_RIGHT:
			cursor_right();
			break;
		default:
			break;
	}
}

static void edit_enigm(unsigned i)
{
	unsigned ei = offset_editable_enigms+i;
	if (ei<nb_editable_enigms) {
		enigm_to_play = ei;
		edit_session = 1;
		cursor_rem();
		init = 0;
		mode_change(MODE_PLAY);
	} else {
		if (lines[52+i].string[3].str[0] != 47) {	/* '.' */
			char *enigm_name;
			unsigned c;
			enigm_to_play = ~0;
			edit_session = 1;
			enigm_name = gltv_memspool_alloc(lines[52+i].string[3].len+1);
			if (!enigm_name) gltv_log_fatal("edit_enigm: Cannot get mem");
			convert_to_ascii(enigm_name, lines[52+i].string[3].str, lines[52+i].string[3].len);
			for (c=lines[52+i].string[3].len-1; c>=0; c--) {
				if (enigm_name[c]!='.') {
					enigm_name[c+1] = '\0';
					break;
				}
			}
			edit_session_name = enigm_name;
			cursor_rem();
			init = 0;
			mode_change(MODE_PLAY);
		} else {
			cursor_moveto(52+i, 3, 0);
		}
	}
}

static void del_editable_enigm(unsigned i)
{
	unsigned ei = offset_editable_enigms+i;
	if (ei<nb_editable_enigms) {
		enigm *e = gltv_slist_get(editable_enigms, ei);
		data_enigm_del(e);
		gltv_slist_remove(editable_enigms, ei);
		cursor_rem();
		nb_editable_enigms--;
		if (offset_editable_enigms>nb_editable_enigms) offset_editable_enigms--;
		update_editable_enigms();
	}
}


static void init_all_enigms()
{
	init_scored_enigms();
	init_free_enigms();
	init_editable_enigms();
}

static void enigm_end_edit(unsigned i)
{
	unsigned ei = offset_editable_enigms+i;
	if (ei<nb_editable_enigms) {
		enigm *e = gltv_slist_get(editable_enigms, ei);
		assert(e!=NULL);
		gltv_slist_remove(editable_enigms, ei);
		gltv_slist_insert(enigms, e);
		init_all_enigms();
	}
}

static void init_lines()
{
	static char lines_ok = 0;
	unsigned x,y;
	if (lines_ok) return;
	for (y=0; y<NB_CHARS_Y; y++) {
		for (x=0; x<lines[y].nb_strings; x++) {
			convert_str(lines[y].string[x].str, lines[y].string[x].len);
			lines[y].string[x].selected=0;
		}
	}
	lines_ok = 1;
}

static void init_pseudo()
{
	unsigned l = strlen(user_name);
	unsigned ll = lines[24].string[2].len;
	unsigned lll;
	static char pseudo_ok = 0;
	if (pseudo_ok) return;
	if (l<ll) lll=l;
	else lll=ll;
	memset(lines[24].string[2].str, ' ', ll-lll);
	memcpy(lines[24].string[2].str+ll-lll, user_name, lll);
	convert_str(lines[24].string[2].str, ll);
	pseudo_ok = 1;
}

static void init_score()
{
	unsigned l = lines[26].string[2].len;
	int i;
	unsigned s = user_score;
	for (i=0; i<l; i++) {
		lines[26].string[2].str[l-1-i] = '0'+(s%10);
		s /= 10;
	}
	convert_str(lines[26].string[2].str, l);
}

static void put_screen()
{
	unsigned x,l;
	int y;
	float offset_x, offset_y;
	int y_min, y_max, x_min, x_max;
	int xp,yp;
	unproj_to_char(-1,1, camera.c, &xp, &yp);
	x_min = x_max = xp;
	y_min = y_max = yp;
	unproj_to_char(1,1, camera.c, &xp, &yp);
	if (xp<x_min) x_min=xp;
	if (xp>x_max) x_max=xp;
	if (yp<y_min) y_min=yp;
	if (yp>y_max) y_max=yp;
	unproj_to_char(1,-1, camera.c, &xp, &yp);
	if (xp<x_min) x_min=xp;
	if (xp>x_max) x_max=xp;
	if (yp<y_min) y_min=yp;
	if (yp>y_max) y_max=yp;
	unproj_to_char(-1,-1, camera.c, &xp, &yp);
	if (xp<x_min) x_min=xp;
	if (xp>x_max) x_max=xp;
	if (yp<y_min) y_min=yp;
	if (yp>y_max) y_max=yp;
	if (x_min<0) x_min=0;
	if (y_min<0) y_min=0;
	y_max++;
	if (x_max>NB_CHARS_X) x_max=NB_CHARS_X;
	if (y_max>NB_CHARS_Y) y_max=NB_CHARS_Y;
	for (y=y_min; y<y_max; y++) {
		for (x=0; x<lines[y].nb_strings; x++) {
			if (lines[y].string[x].pos_x<=x_max && lines[y].string[x].pos_x+(int)lines[y].string[x].len>=x_min) {
				if (!lines[y].string[x].selected) {
					offset_x = offset_y = 0;
				}
				for (l=0; l<lines[y].string[x].len; l++) {
					if (lines[y].string[x].selected) {
						offset_x = SIZE_CHAR*(drand48()-.5)*3./8.;
						offset_y = SIZE_CHAR*(drand48()-.5)*3./8.;
					}
					put_char(l+lines[y].string[x].pos_x, y, lines[y].string[x].str[l], offset_x, offset_y);
				}
			}
		}
	}
}

void intro_display(void)
{
	GLfloat lpos[] = {0,0,.4,1};
	static GLfloat lcol[] = {1,1,1,1};
	if (0==init) {
		glClearColor(0, 0, 0, 0);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glFrustum(-1,1, -1,1, -FRUSTRUM_Z, 10);
		glMatrixMode(GL_MODELVIEW);
		glGetFloatv(GL_PROJECTION_MATRIX, projection);
		geom_set_current_projection(projection);
		glEnable(GL_LIGHTING);
		glEnable(GL_COLOR_MATERIAL);
		glEnable(GL_LIGHT0);
		glShadeModel(GL_SMOOTH);
		glDisable(GL_STENCIL_TEST);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
		glDisable(GL_BLEND);
		glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE);
		glColor4f(1.,1.,1.,1.);
		glEnable(GL_TEXTURE_2D);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_REPEAT);
		glClear(GL_COLOR_BUFFER_BIT);
		if (NULL==dot) dot = data_load_texture("dot.png");
		if (NULL==dot_fill) dot_fill = data_load_texture("dot_fill.png");
		if (NULL==dot || NULL==dot_fill) gltv_log_fatal("mode2: Cannot load textures");
		init_lines();
		target_id = 0;
		init_pseudo();
		init_score();
		init_all_enigms();
		sound_music_start(0);
		quit = 0;
		cursor_str = cursor_line = cursor_x = 0;
		init = 1;
	}
	geom_position_approche(camera.c, target[target_id], 0.03);
	glLoadIdentity();
	glLightfv(GL_LIGHT0, GL_POSITION, lpos);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, lcol);
	if (quit) {
		lcol[0] *= .9;
		lcol[1] *= .9;
		lcol[2] *= .9;
		lcol[3] *= .9;
		if (lcol[0]<.001) {
			init = 0;
			mode_change(MODE_SHUTDOWN);
		}
	}
	{
		/* small oscillations */
		static double angle=0, angle2=0;
		double m = (camera.c[13]<0?1.:-1.)*.004*glut_mouse_x;
		double a2 = .001*sin(angle) - m;
		float c = cos(a2);
		float s = sin(a2);
		float c00 = camera.c[0]*c + camera.c[1]*s;
		float c01 = -camera.c[0]*s + camera.c[1]*c;
		float c10 = camera.c[4]*c + camera.c[5]*s;
		float c11 = -camera.c[4]*s + camera.c[5]*c;
		camera.c[0] = c00;
		camera.c[1] = c01;
		camera.c[4] = c10;
		camera.c[5] = c11;
		angle -= .01;

		m = /*(camera.c[13]<0?1.:-1.)**/.002*glut_mouse_y;
		a2 = .0003*sin(angle) - m;
		c = cos(a2);
		s = sin(a2);
		c00 = camera.c[10]*c + camera.c[9]*s;
		c01 = -camera.c[10]*s + camera.c[9]*c;
		c10 = camera.c[6]*c + camera.c[5]*s;
		c11 = -camera.c[6]*s + camera.c[5]*c;
		camera.c[10] = c00;
		camera.c[9] = c01;
		camera.c[6] = c10;
		camera.c[5] = c11;
		angle2 += .007;
	}
	glLoadMatrixf(camera.c);
	geom_set_current_modelview(camera.c);	/* not this one. TODO : replace current_modelview by a position, so that we could inform geom in real time */
	{
		/* draw background */
		unsigned x, y;
		GLfloat px, py;
		glBindTexture(GL_TEXTURE_2D, dot->binding);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glBegin(GL_QUADS);
		py = (NB_CHARS_Y*.5)*SIZE_CHAR;
		for (y=0; y<NB_QUADS; y++) {
			px = -(NB_CHARS_X*.5)*SIZE_CHAR;
			for (x=0; x<NB_QUADS; x++) {
				glNormal3f(0, 0, 1);
				glTexCoord2f(0,0);
				glVertex3f(px, py, 0);
				glNormal3f(0, 0, 1);
				glTexCoord2f(SIZE_QUAD_TEX_X,0);
				glVertex3f(px+SIZE_QUAD_X, py, 0);
				glNormal3f(0, 0, 1);
				glTexCoord2f(SIZE_QUAD_TEX_X,SIZE_QUAD_TEX_Y);
				glVertex3f(px+SIZE_QUAD_X, py-SIZE_QUAD_Y, 0);
				glNormal3f(0, 0, 1);
				glTexCoord2f(0,SIZE_QUAD_TEX_Y);
				glVertex3f(px, py-SIZE_QUAD_Y, 0);
				px += SIZE_QUAD_X;
			}
			py -= SIZE_QUAD_Y;
		}
		glEnd();
	}
	{
		/* draw texts */
		glBindTexture(GL_TEXTURE_2D, dot_fill->binding);
		put_screen();
	}
	{
		/* "draw" cursor */
		if (glut_mouse_changed) {
			/* non, pas besoin de faire appel à unproject.
			 * il suffit : de récuperer les coordonnées relatives au frustrum : si
			 * de -1,1,-1,1,.4,10 alors mettre les coords de -1 à 1 en x et y,
			 * considérer qu'elles sont à .4 de distance avec C -> on a alors les
			 * coord d'un point sur le coté proche du frustrum.
			 * suffit ensuite de se remettre dans le repère absolut, et projeter
			 * sur le plan.
			 */
			/* as frustrum is -1,1,-1,1, no need to convert from glut_mouse_x & glut_mouse_y, just to inverse y axis (1 for mouse_y is bottom, that is -1 in (0)) */
			int tx, ty;
			unsigned s;
			unproj_to_char(glut_mouse_x, -glut_mouse_y, camera.c, &tx, &ty);
			if (tx>=0 && ty>=0 && tx<NB_CHARS_X && ty<NB_CHARS_Y) {
				/* which string is this ? */
				if (some_selected) {
					lines[selected_y].string[selected_x].selected = 0;
					some_selected = 0;
				}
				for (s=0; s<lines[ty].nb_strings; s++) {
					if (lines[ty].string[s].selectable && tx>=lines[ty].string[s].pos_x && tx<lines[ty].string[s].pos_x+(int)lines[ty].string[s].len) {
						lines[ty].string[s].selected=1;
						selected_x = s;
						selected_y = ty;
						some_selected = 1;
						break;
					}
				}
			}
			glut_mouse_changed=0;
		}
	}
}

void intro_clic(int button, int state)
{
	int s;
	if (!some_selected || state==0) return;
	s = selected_x*1000+selected_y;
	switch (s) {
		/* menu de depart */
		case 4030:	/* quit */
			target_id=1;
			quit=1;
			break;
		case 2030:	/* ok */
			target_id=2;
			break;
		/* menu principal */
		case 3041:	/* retours */
			target_id=0;
			break;
		case 4041:	/* jouer */
			target_id=3;
			break;
		case 2075:	/* tournoi */
			target_id=4;
			break;
		case 2096:	/* retours */
		case 7096:
		case 3059:
		case 4073:
			cursor_rem();
			target_id=2;
			break;
		case 4075:	/* libre */
			target_id=5;
			break;
		case 5041:	/* editer */
			target_id=6;
			break;
		case 6041:	/* partager */
			target_id=7;
			break;
		case 2085:	/* scored enigms */
		case 4085:
			run_scored_enigm(0);
			break;
		case 2086:	/* scored enigms */
		case 4086:
			run_scored_enigm(1);
			break;
		case 2087:	/* scored enigms */
		case 4087:
			run_scored_enigm(2);
			break;
		case 2088:	/* scored enigms */
		case 4088:
			run_scored_enigm(3);
			break;
		case 2089:	/* scored enigms */
		case 4089:
			run_scored_enigm(4);
			break;
		case 2090:	/* scored enigms */
		case 4090:
			run_scored_enigm(5);
			break;
		case 2091:	/* scored enigms */
		case 4091:
			run_scored_enigm(6);
			break;
		case 2092:	/* scored enigms */
		case 4092:
			run_scored_enigm(7);
			break;
		case 2093:	/* scored enigms */
		case 4093:
			run_scored_enigm(8);
			break;
		case 2094:	/* scored enigms */
		case 4094:
			run_scored_enigm(9);
			break;
		case 3052:	/* first editable enigm */
			edit_enigm(0);
			break;
		case 3053:
			edit_enigm(1);
			break;
		case 3054:
			edit_enigm(2);
			break;
		case 3055:
			edit_enigm(3);
			break;
		case 3056:
			edit_enigm(4);
			break;
		case 3057:
			edit_enigm(5);
			break;
		case 7085:
			run_free_enigm(0);
			break;
		case 7086:
			run_free_enigm(1);
			break;
		case 7087:
			run_free_enigm(2);
			break;
		case 7088:
			run_free_enigm(3);
			break;
		case 7089:
			run_free_enigm(4);
			break;
		case 7090:
			run_free_enigm(5);
			break;
		case 7091:
			run_free_enigm(6);
			break;
		case 7092:
			run_free_enigm(7);
			break;
		case 7093:
			run_free_enigm(8);
			break;
		case 7094:
			run_free_enigm(9);
			break;
		case 7052:	/* end the edition of an enigm */
			enigm_end_edit(0);
			break;
		case 7053:
			enigm_end_edit(1);
			break;
		case 7054:
			enigm_end_edit(2);
			break;
		case 7055:
			enigm_end_edit(3);
			break;
		case 7056:
			enigm_end_edit(4);
			break;
		case 7057:
			enigm_end_edit(5);
			break;
		case 5059:	/* right scroll editable enigms */
			editable_enigms_scroll(1);
			break;
		case 4059:	/* left scroll editable enigms */
			editable_enigms_scroll(-1);
			break;
		case 9096:	/* right scroll free enigms */
			free_enigms_scroll(1);
			break;
		case 8096:	/* left scroll free enigms */
			free_enigms_scroll(-1);
			break;
		case 4096:	/* right scroll scored enigms */
			scored_enigms_scroll(1);
			break;
		case 3096:	/* left scroll scored enigms */
			scored_enigms_scroll(-1);
			break;
		case 5052:	/* delete editable enigm */
			del_editable_enigm(0);
			break;
		case 5053:	/* delete editable enigm */
			del_editable_enigm(1);
			break;
		case 5054:	/* delete editable enigm */
			del_editable_enigm(2);
			break;
		case 5055:	/* delete editable enigm */
			del_editable_enigm(3);
			break;
		case 5056:	/* delete editable enigm */
			del_editable_enigm(4);
			break;
		case 5057:	/* delete editable enigm */
			del_editable_enigm(5);
			break;
		default:
			break;
	}
	sound_sample_start(0, 250, 1, 128);
}
