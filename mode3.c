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
#include <assert.h>
#include <string.h>
#include <math.h>
#include "modes.h"
#include "data.h"
#include "slist.h"
#include "csg.h"
#include "log.h"
#include "geom.h"
#include "draw.h"
#include "data.h"
#include "main.h"
#include "mode2.h" /* for digital font */
#include "user.h"
#include "sound.h"

#ifndef M_PI
#define M_PI       3.14159265358979323846
#endif

/* lets R0ck n r0ll !! */

int edit_session;
char *edit_session_name;

static GLfloat projection[16];
static position camera = {
	{
		1,0,0,0,
		0,1,0,0,
		0,0,1,0,
		0,0,-10,1
	},
	NULL,
	0,
	"camera",
	0,
};

static enigm *e;
static char proposition[NB_MAX_PRIMS_IN_TREE*5];
static char copy_proposition[NB_MAX_PRIMS_IN_TREE*5];
static csg_node *csg_proposition = NULL;
static char proposition_changed;
int nb_tokens;
primitive *prim_operator = NULL;
static double angle_horiz=0, angle_vert=0, distance=7;

unsigned rotation_step;	/* from 0 to M_PI/2, then 0. rotation if > 0 */
unsigned translation_step;	/* from 0 to 1, then 0. translation if > 0 */
enum { TRANS_X, TRANS_Y, TRANS_Z, TRANS_MX, TRANS_MY, TRANS_MZ, ROT_X, ROT_Y, ROT_Z } current_transformation;

static float drag_start_x, drag_stop_x, drag_start_y, drag_stop_y, paste_x, paste_y;
static char drag_on, drag_new, paste_new, camera_rotation_on;
static unsigned selected_prim;	/* if a single primitive if selected */
static int selection_start, selection_stop;
static enum { TRANSFO_TRANSLATE, TRANSFO_ROTATE, TRANSFO_SCALE } transformation;

GLfloat pos_save[NB_MAX_PRIMS_IN_TREE][16];

static struct {
	char *file;
	texture *tex;
} background[] = {
	{ "big1.png", NULL },
	{ "big2.png", NULL },
	{ "big3.png", NULL }
};
#define NB_BACKPIC (sizeof(background)/sizeof(*background))
static struct {
	char *file;
	texture *tex;
} flower[] = {
	{ "titefleur1.png", NULL },
	{ "titefleur3.png", NULL },
	{ "titefleur2.png", NULL },
	{ "titefleur4.png", NULL }
};
#define NB_FLOWERS (sizeof(flower)/sizeof(*flower))

struct {
	const char *file;
	mesh *m;
} mesh_to_load[] = {
	{ "arrow.mesh", NULL },
	{ "instance0_arrow.mesh", NULL },
	{ "instance1_arrow.mesh", NULL },
	{ "instance2_arrow.mesh", NULL },
	{ "instance3_arrow.mesh", NULL },
	{ "instance4_arrow.mesh", NULL },
	{ "curved_arrow.mesh", NULL },
	{ "instance6_curved_arrow.mesh", NULL },
	{ "instance7_curved_arrow.mesh", NULL },
	{ "instance8_curved_arrow.mesh", NULL },
	{ "instance9_curved_arrow.mesh", NULL },	// 10
	{ "instance10_curved_arrow.mesh", NULL },
	{ "smaller_arrow.mesh", NULL },
	{ "instance11_smaller_arrow.mesh", NULL },
	{ "instance12_smaller_arrow.mesh", NULL },
	{ "taller_arrow.mesh", NULL },
	{ "instance13_taller_arrow.mesh", NULL },
	{ "instance14_taller_arrow.mesh", NULL },
	{ "cube_rond.mesh", NULL },
	{ "validate.mesh", NULL },
	{ "parenth.mesh", NULL },	// 20
	{ "back.mesh", NULL },
	{ "solution.mesh", NULL },
	{ "sphere.mesh", NULL },
	{ "zap.mesh", NULL },
	{ "play.mesh", NULL }
};
static struct {
	char aff;
	unsigned mesh_idx;
} icons[] = {
	{ 0, 19 } /* validate */,
	{ 0, 20 } /* set parentheses */,
	{ 0, 21 } /* back to menu */,
	{ 0, 22 } /* view solution */,
	{ 0, 24 } /* zap selection */,
	{ 0, 25 } /* play */
};

position *pos_translate=NULL, *pos_rotate=NULL, *pos_scale=NULL;

void save_positions() {
	/* save actual positions and set primitives at the good location */
	unsigned u;
	for (u=0; u<e->nb_bricks; u++) {
		memcpy(pos_save[u], e->bricks[u]->pos->c, sizeof(pos_save[0]));
		memcpy(e->bricks[u]->pos->c, e->solution_pos[u], sizeof(pos_save[0]));
	}
}

void restore_positions() {
	/* restore the position as the player set them */
	unsigned u;
	for (u=0; u<e->nb_bricks; u++) {
		memcpy(e->bricks[u]->pos->c, pos_save[u], sizeof(pos_save[0]));
	}
}

#define ICONS_SIZE 2.
#define ICONS_SPACING (ICONS_SIZE*1.1)
#define ICONS_DEPTH (ICONS_SIZE*5.)
#define ICONS_START (ICONS_DEPTH-ICONS_SIZE)
#define ICONS_SCREEN_START (1.-ICONS_SIZE*.5/ICONS_DEPTH)
#define ICONS_SCREEN_SPACING (ICONS_SPACING/ICONS_DEPTH)
unsigned icon_clicked(float x, float y) {
	/* tells wich icon is at x,y, or ~0 if none */
	unsigned u, uu;
	int c_x;
	if (y>.6) {
		c_x = (ICONS_SCREEN_START-x)/ICONS_SCREEN_SPACING;	/* approx */
		if (c_x>=0 && c_x<sizeof(icons)/sizeof(*icons)) {
			uu = 0;
			for (u=0; u<sizeof(icons)/sizeof(*icons); u++) {
				if (icons[u].aff) {
					if (c_x==uu) return u;
					uu++;
				}
			}
		}
	}
	return ~0;
}

static void put_char(char ch, GLfloat o_x, GLfloat o_y, GLfloat dot_size) {	/* c is already converted */
	GLfloat px, py, px_init;
	unsigned i,j;
	unsigned char *digit;
	static double angle = 0;
	double s, c;
	dot_size *= .5;
	s = sin(angle)*dot_size;
	c = cos(angle)*dot_size;
	angle += .03;
	assert(c<=47);
	px_init = o_x - .5;
	digit = digital_bits+8*(unsigned)ch;
	glBegin(GL_QUADS);
	py = o_y - .5;
	for (j=0; j<8; j++) {
		px = px_init;
		for (i=0; i<8; i++) {
			if ((digit[j]>>i)&1) {
				glTexCoord2f(0,0);
				glNormal3f(0, 0, 1);
				glVertex3f(px+c, py+s, 0);
				glTexCoord2f(0,1);
				glNormal3f(0, 0, 1);
				glVertex3f(px-s, py+c, 0);
				glTexCoord2f(1,1);
				glNormal3f(0, 0, 1);
				glVertex3f(px-c, py-s, 0);
				glTexCoord2f(1,0);
				glNormal3f(0, 0, 1);
				glVertex3f(px+s, py-c, 0);
			}
			px += 1./8.;
		}
		py -= 1./8.;
	}
	glEnd();
}

void switch_transfo() {
	switch (transformation) {
		case TRANSFO_TRANSLATE:
			transformation = TRANSFO_ROTATE;
			break;
		case TRANSFO_ROTATE:
			transformation = TRANSFO_TRANSLATE; //TRANSFO_SCALE;
			break;
		case TRANSFO_SCALE:
			transformation = TRANSFO_TRANSLATE;
	}
}

int parenth_match(int c) {
	int cc, p_count;
	if (proposition[c]=='(') {
		p_count=1;
		for (cc=c+1; proposition[cc]!='\0' && p_count!=0; cc++) {
			if (proposition[cc]=='(') p_count++;
			else if (proposition[cc]==')') p_count--;
		}
		cc--;
	} else {
		p_count=-1;
		for (cc=c-1; cc>=0 && p_count!=0; cc--) {
			if (proposition[cc]=='(') p_count++;
			else if (proposition[cc]==')') p_count--;
		}
		cc++;
	}
	if (0!=p_count) return ~0;
	else return cc;
}

void toggle_parenth() {
	int i;
	if (parenth_match(selection_start)==selection_stop) {	/* remove ? */
		for (i=selection_start; i<selection_stop-1; i++)
			proposition[i] = proposition[i+1];
		for (; i<nb_tokens-1; i++)
			proposition[i] = proposition[i+2];
		nb_tokens -= 2;
		selection_stop -= 2;
	} else if (selection_stop>selection_start) {	/* add ! */
		for (i=nb_tokens; i>=selection_stop+1; i--)
			proposition[i+2] = proposition[i];
		proposition[i+2] = ')';
		for (; i>=selection_start; i--)
			proposition[i+1] = proposition[i];
		proposition[i+1] = '(';
		nb_tokens += 2;
		selection_stop += 2;
	}
}

int paste(int sel_start, int sel_stop, int to) {
	int c;
	int len = sel_stop-sel_start+1;
	memcpy(copy_proposition, proposition+sel_start, len);
	if (to<sel_start) {
		for (c=sel_start-1; c>=to; c--)
			proposition[c+len] = proposition[c];
		for (c=0; c<len; c++)
			proposition[to+c] = copy_proposition[c];
		return to-sel_start;
	} else {
		for (c=sel_stop+1; c<to; c++)
			proposition[c-len] = proposition[c];
		for (c=0; c<len; c++)
			proposition[to-len+c] = copy_proposition[c];
		return to-len-sel_start;
	}
}

void translate(unsigned prim_idx, mesh *clicked) {
	if (clicked == mesh_to_load[2].m) {
		current_transformation = TRANS_MX;
		translation_step = 1;
	} else if (clicked == mesh_to_load[1].m) {
		current_transformation = TRANS_X;
		translation_step = 1;
	} else if (clicked == mesh_to_load[0].m) {
		current_transformation = TRANS_Y;
		translation_step = 1;
	} else if (clicked == mesh_to_load[3].m) {
		current_transformation = TRANS_MY;
		translation_step = 1;
	} else if (clicked == mesh_to_load[4].m) {
		current_transformation = TRANS_Z;
		translation_step = 1;
	} else if (clicked == mesh_to_load[5].m) {
		current_transformation = TRANS_MZ;
		translation_step = 1;
	}
}

void rotate(unsigned prim_idx, mesh *clicked) {
	if (clicked == mesh_to_load[6].m || clicked == mesh_to_load[7].m) {
		current_transformation = ROT_X;
		rotation_step = 1;
	} else if (clicked == mesh_to_load[8].m || clicked == mesh_to_load[9].m) {
		current_transformation = ROT_Z;
		rotation_step = 1;
	} else if (clicked == mesh_to_load[10].m || clicked == mesh_to_load[11].m) {
		current_transformation = ROT_Y;
		rotation_step = 1;
	}
}

void scale(unsigned prim_idx, mesh *clicked) {
}

int is_operator(char c) {
	return c==operator[CSG_UNION] || c==operator[CSG_AND] || c==operator[CSG_MINUS];
}

int is_prim(char c) {
	return c>='a' && c<='z';
}

void proposition_simplify() {
	/* remove erroneous operators (after paste) or
	 * useless parenthesis (after add parenthesis),
	 * and add union if missing operators
	 */
	int c, cc;
	int encore;
	do {
		encore = 0;
		if (proposition[0]=='(' && parenth_match(0)==nb_tokens-1) {
			for (cc=1; cc<nb_tokens-1; cc++)
				proposition[cc-1] = proposition[cc];
			proposition[cc-1] = '\0';
			nb_tokens -= 2;
			encore = 1;
		}
		for (c=0; c<nb_tokens-1; c++) {
			if ((proposition[c]=='(' && is_operator(proposition[c+1])) || (is_operator(proposition[c]) && proposition[c+1]==')')) {
				char temp = proposition[c];
				proposition[c] = proposition[c+1];
				proposition[c+1] = temp;
				encore = 1;
			}
		}
		for (c=0; c<nb_tokens-1; c++) {
			if (proposition[c]=='(') {
				if (proposition[c+1]==')') {
					for (cc=c; cc<=nb_tokens-2; cc++)
						proposition[cc] = proposition[cc+2];
					nb_tokens -= 2;
					encore = 1;
				} else if (c<nb_tokens-2 && proposition[c+2]==')') {
					proposition[c] = proposition[c+1];
					for (cc=c+1; cc<=nb_tokens-2; cc++)
						proposition[cc] = proposition[cc+2];
					nb_tokens -= 2;
					encore = 1;
				}
			}
		}
		if (is_operator(proposition[0])) {
			for (cc=0; cc<nb_tokens; cc++)
				proposition[cc] = proposition[cc+1];
			nb_tokens--;
			encore = 1;
		}
		if (is_operator(proposition[nb_tokens-1])) {
			proposition[nb_tokens-1] = '\0';
			nb_tokens--;
			encore = 1;
		}
		for (c=1; c<nb_tokens-2; c++) {
			if (is_operator(proposition[c]) && is_operator(proposition[c+1])) {
				for (cc=c+1; cc<=nb_tokens-1; cc++)
					proposition[cc] = proposition[cc+1];
				encore = 1;
				nb_tokens--;
			}
		}
		for (c=0; c<nb_tokens-1; c++) {
			if (
					(is_prim(proposition[c]) && is_prim(proposition[c+1])) ||
					(is_prim(proposition[c]) && proposition[c+1]=='(') ||
					(proposition[c]==')' && is_prim(proposition[c+1]))
				) {
				for (cc=nb_tokens+1; cc>=c+2; cc--)
					proposition[cc] = proposition[cc-1];
				proposition[c+1] = operator[CSG_UNION];
				encore = 1;
				nb_tokens++;
			}
		}
		for (c=0; c<nb_tokens; c++) {
			if ((proposition[c]=='(' || proposition[c]==')') && parenth_match(c)==~0) {
				for (cc=c; cc<nb_tokens; cc++)
					proposition[cc] = proposition[cc+1];
				encore = 1;
				nb_tokens--;
			}
		}
		for (c=0; c<nb_tokens-2; c++) {
			if (proposition[c]=='(' && proposition[c+1]=='(') {
				int pm_1 = parenth_match(c);
				int pm_2 = parenth_match(c+1);
				assert(pm_1!=~0 && pm_2!=~0);
				if (pm_1 == pm_2+1) {
					for (cc=c; cc<pm_2; cc++)
						proposition[cc] = proposition[cc+1];
					for (; cc<nb_tokens-1; cc++)
						proposition[cc] = proposition[cc+2];
					encore = 1;
					nb_tokens -= 2;
				}
			}
		}
		if (encore) {
			selection_stop=selection_start=0;
		}
	} while (encore);
}

void play_display() {
	static csg_union_of_partial_products uopp_sol;
	static csg_union_of_products uop_sol;
	static csg_union_of_partial_products uopp;
	static csg_union_of_products uop;
	static b_box csg_box;
	static double prim_pulse;
	static GLfloat PRIM_DEPTH;
	static double visio_perturb;
	static char wana_view_solution, view_solution;
	static int view_solution_anim;
	static char quit;
	static char init = 0;
	static char timer[3];
	static unsigned utimer, the_flower;
	unsigned u;
	if (!init) {
		unsigned p;
		GLfloat lpos[] = {0,0,.4,1};
		if (!edit_session) {
			assert(enigm_to_play<gltv_slist_size(enigms));
			sound_music_start(1+enigm_to_play%(nb_musics-1));
			e = gltv_slist_get(enigms, enigm_to_play);
			gltv_log_warning(GLTV_LOG_DEBUG, "gona play enigm %u, %s", enigm_to_play, e->name);
			if (!csg_union_of_products_reset(&uop_sol, e->root) || !csg_union_of_partial_products_reset(&uopp_sol, &uop_sol)) {
				gltv_log_fatal("play_display: Cannot get union of products");
			}
		} else {
			if (~0==enigm_to_play) {
				/* build a false enigm with all primitives */
				e = data_enigm_full_new(edit_session_name);
				gltv_log_warning(GLTV_LOG_DEBUG, "new enigm named '%s'", edit_session_name);
			} else {
				/* update of an existing enigm */
				assert(enigm_to_play<gltv_slist_size(editable_enigms));		
				e = gltv_slist_get(editable_enigms, enigm_to_play);
				gltv_log_warning(GLTV_LOG_DEBUG, "gona edit enigm %u, %s", enigm_to_play, e->name);
			}
		}
		if (NULL==prim_operator) prim_operator = data_load_primitive("operator.prim", 1);
		glClearColor(0.18, 0.28, 0.28, 0.0);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glFrustum(-1,1, -1,1, 1,100);
		glMatrixMode(GL_MODELVIEW);
		glGetFloatv(GL_PROJECTION_MATRIX, projection);
		geom_set_current_projection(projection);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_LIGHTING);
		glEnable(GL_LIGHT0);
		glEnable(GL_COLOR_MATERIAL);
		glShadeModel(GL_SMOOTH);
		glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
		glLoadIdentity();
		glLightfv(GL_LIGHT0, GL_POSITION, lpos);
		glDisable(GL_TEXTURE_2D);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		if (edit_session && ~0!=enigm_to_play) {
			/* in this case we start at the solution */
			nb_tokens = strlen(e->solution);
			memcpy(proposition, e->solution, nb_tokens+1);
		} else {
			for (p=0, u=0; u<e->nb_bricks; u++) {
				geom_set_identity(e->bricks[u]->pos->c);
				if (u>0) {
					proposition[p++] = operator[CSG_UNION];
				}
				proposition[p++] = u+'a';
			}
			proposition[p] = '\0';
			nb_tokens = p;
		}
		save_positions();	/* init savepos to identity & set e->bricks[u]->pos->c to e->solution_pos */
		proposition_changed = 1;
		rotation_step = 0;
		translation_step = 0;
		selection_start = selection_stop = 0;
		selected_prim = ~0;
		drag_on = 0;
		drag_new = 0;
		paste_new = 0;
		prim_pulse = 0.;
		camera_rotation_on = 0;
		if (!edit_session) {
			wana_view_solution = 1;
			view_solution = 1;
			view_solution_anim = 21;
		} else {
			wana_view_solution = 0;
			view_solution = 0;
			view_solution_anim = -1;
		}
		quit = 0;
		visio_perturb = 0;
		PRIM_DEPTH = -20.;
		transformation = TRANSFO_TRANSLATE;
		for (u=0; u<sizeof(mesh_to_load)/sizeof(*mesh_to_load); u++) {
			if (NULL==mesh_to_load[u].m) {
				mesh_to_load[u].m = data_load_mesh(mesh_to_load[u].file, 0);
				if (NULL==mesh_to_load[u].m) {
					gltv_log_fatal("play_display: Cannot load mesg '%s'", mesh_to_load[u].file);
				}
			}
		}
		if (pos_translate==NULL) pos_translate = data_get_named_position("translate.pos");
		if (pos_rotate==NULL) pos_rotate = data_get_named_position("rotate.pos");
		if (pos_scale==NULL) pos_scale = data_get_named_position("scale.pos");
		for (u=0; u<NB_FLOWERS; u++) {
			if (NULL==flower[u].tex) flower[u].tex = data_load_texture(flower[u].file);
		}
		the_flower = enigm_to_play!=~0?enigm_to_play%NB_FLOWERS:0;
		for (u=0;
				u<NB_BACKPIC;
				u++) {
			if (NULL==background[u].tex) background[u].tex = data_load_texture(background[u].file);
		}
		mesh_to_load[23].m->texture = background[enigm_to_play!=~0?enigm_to_play%NB_BACKPIC:0].tex;
		timer[0] = 4;
		timer[1] = 0;
		timer[2] = 0;
		utimer = glut_interval;
		init=1;
	}
	{	/* resolve csg proposal */
		if (proposition_changed) {
			gltv_log_warning(GLTV_LOG_DEBUG, "proposition = '%s'\n",proposition);
			if (NULL!=csg_proposition) csg_node_del(csg_proposition);
			memcpy(copy_proposition, proposition, nb_tokens+1);
			csg_proposition = csg_build_tree(copy_proposition, e->nb_bricks, e->bricks);
			if (NULL!=csg_proposition) {
				if (!csg_union_of_products_reset(&uop, csg_proposition) || !csg_union_of_partial_products_reset(&uopp, &uop)) {	/* build uop(p) */
					gltv_log_fatal("play_display: Cannot reset uop and/or uopp");
				}
			}
			csg_get_b_box(csg_proposition, &csg_box);
			proposition_changed = 0;
		}
	}
	{	/* move prims */
		if (rotation_step!=0) {
			GLfloat rot[16], res[16];
			double angle = M_PI*-.05;
			double c = cos(angle);
			double s = sin(angle);
			geom_set_identity(rot);
			switch (current_transformation) {
				case ROT_X:
					rot[5] = c;
					rot[6] = s;
					rot[9] = -s;
					rot[10] = c;
					break;
				case ROT_Y:
					rot[0] = c;
					rot[2] = s;
					rot[8] = -s;
					rot[10] = c;
					break;
				case ROT_Z:
					s = -s;
					rot[0] = c;
					rot[1] = s;
					rot[4] = -s;
					rot[5] = c;
					break;
				default:
					gltv_log_fatal("play_display: transformations fucked up");
			}
			geom_matrix_mult(res, e->bricks[selected_prim]->pos->c, rot);
			memcpy(e->bricks[selected_prim]->pos->c, res, sizeof(res));
			if (rotation_step++==10) {
				rotation_step=0;
			}
			proposition_changed = 1;
		} else if (translation_step!=0) {
			switch (current_transformation) {
				case TRANS_X:
					e->bricks[selected_prim]->pos->c[12] += .1*e->bricks[selected_prim]->pos->c[0];
					e->bricks[selected_prim]->pos->c[13] += .1*e->bricks[selected_prim]->pos->c[1];
					e->bricks[selected_prim]->pos->c[14] += .1*e->bricks[selected_prim]->pos->c[2];
					break;
				case TRANS_MX:
					e->bricks[selected_prim]->pos->c[12] -= .1*e->bricks[selected_prim]->pos->c[0];
					e->bricks[selected_prim]->pos->c[13] -= .1*e->bricks[selected_prim]->pos->c[1];
					e->bricks[selected_prim]->pos->c[14] -= .1*e->bricks[selected_prim]->pos->c[2];
					break;
				case TRANS_Y:
					e->bricks[selected_prim]->pos->c[12] += .1*e->bricks[selected_prim]->pos->c[4];
					e->bricks[selected_prim]->pos->c[13] += .1*e->bricks[selected_prim]->pos->c[5];
					e->bricks[selected_prim]->pos->c[14] += .1*e->bricks[selected_prim]->pos->c[6];
					break;
				case TRANS_MY:
					e->bricks[selected_prim]->pos->c[12] -= .1*e->bricks[selected_prim]->pos->c[4];
					e->bricks[selected_prim]->pos->c[13] -= .1*e->bricks[selected_prim]->pos->c[5];
					e->bricks[selected_prim]->pos->c[14] -= .1*e->bricks[selected_prim]->pos->c[6];
					break;
				case TRANS_Z:
					e->bricks[selected_prim]->pos->c[12] += .1*e->bricks[selected_prim]->pos->c[8];
					e->bricks[selected_prim]->pos->c[13] += .1*e->bricks[selected_prim]->pos->c[9];
					e->bricks[selected_prim]->pos->c[14] += .1*e->bricks[selected_prim]->pos->c[10];
					break;
				case TRANS_MZ:
					e->bricks[selected_prim]->pos->c[12] -= .1*e->bricks[selected_prim]->pos->c[8];
					e->bricks[selected_prim]->pos->c[13] -= .1*e->bricks[selected_prim]->pos->c[9];
					e->bricks[selected_prim]->pos->c[14] -= .1*e->bricks[selected_prim]->pos->c[10];
					break;
				default:
					gltv_log_fatal("play_display: transformations fucked up");
			}
			if (translation_step++==10) {
				translation_step=0;
			}
			proposition_changed = 1;
		}
	}
	{ /* background */
		static GLfloat pos_wallbox[16] = {0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,-15,1};
		static float factor = 100.;
		glClear(GL_COLOR_BUFFER_BIT);
		pos_wallbox[0] = camera.c[0]*factor;
		pos_wallbox[1] = camera.c[1]*factor;
		pos_wallbox[2] = camera.c[2]*factor;
		pos_wallbox[4] = camera.c[4]*factor;
		pos_wallbox[5] = camera.c[5]*factor;
		pos_wallbox[6] = camera.c[6]*factor;
		pos_wallbox[8] = camera.c[8]*factor;
		pos_wallbox[9] = camera.c[9]*factor;
		pos_wallbox[10] = camera.c[10]*factor;
		glDisable(GL_CULL_FACE);
		glDisable(GL_BLEND);
		glDisable(GL_DEPTH_TEST);
		glLoadMatrixf(pos_wallbox);
		draw_mesh(mesh_to_load[23].m);
	}
	glStencilMask(~0);
	glDepthMask(1);
	glColorMask(1,1,1,1);
	glClear(GL_DEPTH_BUFFER_BIT);
	glColor3f(1.,1.,1.);
	if (wana_view_solution) {
		if (view_solution_anim<20) view_solution_anim++;
		if (view_solution_anim>10) {
			view_solution = 1;
		}
		if (view_solution_anim==20) {
			save_positions();
			view_solution_anim++;	/* so we wont save several times */
		}
	} else {
		if (view_solution_anim>0) view_solution_anim--;
		if (view_solution_anim<10) {
			view_solution = 0;
		}
		if (view_solution_anim==0) {
			restore_positions();
			view_solution_anim--;
		}
	}
	if (view_solution) {
		if (!wana_view_solution) {
			camera.c[13] += (20-view_solution_anim)*.6;
		}
		glLoadMatrixf(camera.c);
		geom_set_current_modelview(camera.c);
		draw_union_of_partial_products(&uopp_sol);	/* solution */
	} else {
		static double angle_f = 0;
		static double angle_x = 0;
		static double angle_y = 0;
		if (visio_perturb>=0) {
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glFrustum(-1+sin(angle_x)*visio_perturb,1.-sin(angle_x+3.)*visio_perturb, -1+sin(angle_y)*visio_perturb,1-sin(angle_y+3.)*visio_perturb, 1.+sin(angle_f)*visio_perturb,100);
			angle_f += .4;
			angle_x += .43;
			angle_y += .61;
			glMatrixMode(GL_MODELVIEW);
			glGetFloatv(GL_PROJECTION_MATRIX, projection);
			geom_set_current_projection(projection);
		}
		if (wana_view_solution) {
			camera.c[13] += view_solution_anim*.6;
		}
		glLoadMatrixf(camera.c);
		geom_set_current_modelview(camera.c);
		if (NULL!=csg_proposition) draw_union_of_partial_products(&uopp);	/* proposal */
		if (visio_perturb>=0) {
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glFrustum(-1,1, -1,1, 1,100);
			glMatrixMode(GL_MODELVIEW);
			glGetFloatv(GL_PROJECTION_MATRIX, projection);
			geom_set_current_projection(projection);
		}
		if (!quit)
			visio_perturb -= .001;
		else {
			visio_perturb += .03;
		}
	}
	{	/* move camera */
		double s_h, c_h, s_v, c_v;
		GLfloat CO[3];	/* vector from center of the world to camera */
		GLfloat Ct[3];	/* target for Ct */
		c_h = cos(angle_horiz);
		s_h = sin(angle_horiz);
		c_v = cos(angle_vert);
		s_v = sin(angle_vert);
		camera.c[0] = -s_h;	/* Cx */
		camera.c[4] = c_h;
		camera.c[8] = 0;
		camera.c[1] = -c_h*s_v;	/* Cy */
		camera.c[5] = -s_h*s_v;
		camera.c[9] = c_v;
		camera.c[2] = c_h*c_v;	/* Cz */
		camera.c[6] = s_h*c_v;
		camera.c[10] = s_v;
		if (csg_box.vide || view_solution) {
			for (u=0; u<3; u++) {
				CO[u] = -distance*camera.c[2+(u<<2)];
			}
		} else {
			for (u=0; u<3; u++) {	/* CO in world coordinates */
				CO[u] = (csg_box.p_max[u]+csg_box.p_min[u])*-.5 - distance*camera.c[2+(u<<2)];
			}
		}
		for (u=0; u<3; u++) {	/* T = OC in camera's coordinates */
			unsigned i;
			Ct[u] = 0;
			for (i=0; i<3; i++) {
				Ct[u] += camera.c[u+(i<<2)]*CO[i];
			}
			camera.c[12+u] += (Ct[u]-camera.c[12+u])*.1;
		}
	}
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_STENCIL_TEST);
	glEnable(GL_CULL_FACE);
	glColorMask(1,1,1,1);
	{ /* draw transformators */
		if (selected_prim != ~0 && !view_solution) {
			GLfloat prim_alpha;
			position *pos;
			switch (transformation) {
				case TRANSFO_TRANSLATE:
					pos = pos_translate;
					break;
				case TRANSFO_ROTATE:
					pos = pos_rotate;
					break;
				case TRANSFO_SCALE:
					pos = pos_scale;
					break;
			}
			memcpy(pos->c, e->bricks[selected_prim]->pos->c, sizeof(pos->c));
			glEnable(GL_LIGHTING);
			glEnable(GL_BLEND);
			glColor3f(1.,.1,.3);//,.7);
			draw_position(pos);
			prim_alpha = .6*sin(prim_pulse);
			if (prim_alpha>0) {
				glColor3f(.1,.2,.9);//,prim_alpha);
				draw_position(e->bricks[selected_prim]->pos);
			}
			prim_pulse += .5;
			glDisable(GL_BLEND);
		}
	}
#define PRIM_HEIGHT 0. /*(-PRIM_DEPTH-3.)*/
#define PRIM_LEFT (PRIM_DEPTH+2.)
#define PRIM_SPACING (4.)
#define PRIM_SCREEN_SPACING (PRIM_SPACING/-PRIM_DEPTH)
#define PRIM_SCREEN_LEFT ((PRIM_LEFT-PRIM_SPACING*.5)/-PRIM_DEPTH)
	{
		GLfloat new_depth = -2.*nb_tokens;
		if (new_depth != PRIM_DEPTH) {
			PRIM_DEPTH += (new_depth-PRIM_DEPTH)*.1;
			if (PRIM_DEPTH>-20.) PRIM_DEPTH=-20;
		}
	}
	{	/* mouse events */
		if (drag_new) {
			if (camera_rotation_on) {
				camera_rotation_on = 0;
			} else if (drag_start_y<-.6 && !view_solution) {
				if (selection_start==selection_stop) {
					/* action ! look for the prim */
					if (proposition[selection_start]==operator[CSG_UNION]) {
						proposition[selection_start] = operator[CSG_AND];
						selected_prim = ~0;
						proposition_changed = 1;
					} else if (proposition[selection_start]==operator[CSG_AND]) {
						proposition[selection_start] = operator[CSG_MINUS];
						selected_prim = ~0;
						proposition_changed = 1;
					} else if (proposition[selection_start]==operator[CSG_MINUS]) {
						proposition[selection_start] = operator[CSG_UNION];
						selected_prim = ~0;
						proposition_changed = 1;
					} else if (proposition[selection_start]=='(') {
						selection_stop = parenth_match(selection_start);
					} else if (proposition[selection_start]==')') {
						selection_start = parenth_match(selection_start);
					} else {
						unsigned new_selected_prim = proposition[selection_start]-'a';
						if (selected_prim == new_selected_prim) {
							switch_transfo();
						} else {
							selected_prim = new_selected_prim;
							assert(selected_prim<e->nb_bricks);
						}
					}
				} else {
					selected_prim = ~0;
				}
			} else if (~0!=(u=icon_clicked(drag_stop_x, drag_stop_y))) {
				sound_sample_start(1, 250, 1, 200);
				switch (u) {
					case 0:	/* validate */
						if (csg_is_equivalent(&uopp, &uopp_sol) && geom_pos_is_equivalent(e->bricks, e->solution_pos, e->nb_bricks)) {
							gltv_log_warning(GLTV_LOG_DEBUG, "VICTORY");
							user_score += e->score;
							quit = 1;
						} else {
							visio_perturb = .1;
							sound_sample_start(4, 250, 1, 128);
						}
						break;
					case 1:	/* parenth */
						toggle_parenth();
						proposition_simplify();
						proposition_changed = 1;
						break;
					case 2:	/* back to menu */
						quit = 1;
						visio_perturb = .001;
						break;
					case 5:	/* play */
					case 3:	/* view_solution */
						if (!wana_view_solution) {
							wana_view_solution = 1;
						} else {
							wana_view_solution = 0;
						}
						break;
					case 4:	/* zap */
						if (selection_start>0 || selection_stop<nb_tokens-2) {
							memmove(proposition+selection_start, proposition+selection_stop+1, nb_tokens-selection_stop);
							nb_tokens -= selection_stop-selection_start+1;
							proposition_simplify();
							selection_start = selection_stop = 0;
							selected_prim = ~0;
							proposition_changed = 1;
						}
						break;
				}
			} else if (!rotation_step && !translation_step && selected_prim!=~0 && !view_solution) {	/* test the transformators */
				mesh *clicked;
				GLfloat z;
				switch (transformation) {
					case TRANSFO_TRANSLATE:	
						clicked = geom_clic_position(pos_translate, &z);
						if (NULL!=clicked) translate(selected_prim, clicked);
						break;
					case TRANSFO_ROTATE:
						clicked = geom_clic_position(pos_rotate, &z);
						if (NULL!=clicked) rotate(selected_prim, clicked);
						break;
					case TRANSFO_SCALE:
						clicked = geom_clic_position(pos_scale, &z);
						if (NULL!=clicked) scale(selected_prim, clicked);
						break;
				}
			}
			drag_new = 0;
		} else if (drag_on) {
			if (drag_start_y<-.6 && !view_solution) {
				int start = (drag_start_x-PRIM_SCREEN_LEFT)/PRIM_SCREEN_SPACING;
				int stop = (drag_stop_x-PRIM_SCREEN_LEFT)/PRIM_SCREEN_SPACING;
				if (stop<start) {
					int temp = stop;
					stop=start;
					start=temp;
				}
				if (start<0) start = 0;
				if (stop>=nb_tokens) stop=nb_tokens-1;
				selection_start = start;
				selection_stop = stop;
			}
		} else if (paste_new) {
			if (paste_y<-.6 && !view_solution) {
				int paste_token = (paste_x-PRIM_SCREEN_LEFT)/PRIM_SCREEN_SPACING;
				if (paste_token<0) paste_token = 0;
				if (paste_token>=nb_tokens) paste_token=nb_tokens;	/* we paste BEFORE the token */
				if (selection_start<=paste_token && paste_token<=selection_stop+1) {	/* add parentheses */
					toggle_parenth();
				} else {	/* cut & paste */
					int delta = paste(selection_start, selection_stop, paste_token);
					selection_start += delta;
					selection_stop += delta;
				}
				proposition_simplify();
				selected_prim = ~0;
				proposition_changed = 1;
				sound_sample_start(3, 250, 1, 128+(selection_start+selection_stop-nb_tokens));
			}
			paste_new = 0;
		}
	}
	{	/* draw menu */
		
		GLfloat primpos[16];
		unsigned p;
		int paste_token;
		GLfloat h_pixel = glut_fenHaut*.2, h_pixel_f;
		glViewport(0,glut_fenHaut-h_pixel,glut_fenLong,h_pixel);
		h_pixel_f = h_pixel/glut_fenLong;
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glFrustum(-1,1, -h_pixel_f,h_pixel_f, 1,100);
		glMatrixMode(GL_MODELVIEW);
		primpos[3] = primpos[7] = primpos[11] = 0;
		primpos[13] = PRIM_HEIGHT;
		primpos[14] = PRIM_DEPTH;
		primpos[15] = 1;
		glDisable(GL_LIGHTING);
		glLoadIdentity();
		glEnable(GL_BLEND);
		glBegin(GL_QUADS);	// background
		glColor3f(.1,.2,.5);//,.3);
		glVertex3f(-1,-1,-1.);
		glVertex3f(1,-1,-1.);
		glVertex3f(1,1,-1.);
		glVertex3f(-1,1,-1.);
		glEnd();
		glDisable(GL_BLEND);
		/* prims, ops and parenths */
		primpos[12] = PRIM_LEFT;
		geom_set_current_modelview(primpos);
		glEnable(GL_LIGHTING);
		paste_token = (glut_mouse_x-PRIM_SCREEN_LEFT)/PRIM_SCREEN_SPACING;
		if (paste_token<0) paste_token = 0;
		if (paste_token>=nb_tokens) paste_token=nb_tokens;
		if (glut_mouse_y>-.6 || (paste_token>=selection_start && paste_token<=selection_stop+1) || view_solution) paste_token=-1;
		for (p=0; proposition[p]!='\0'; p++) {
			if (p==paste_token-1) {
				primpos[12] -= PRIM_SPACING*.2;
			} else if (p==paste_token) {
				primpos[12] += PRIM_SPACING*.2;
			}
			if (proposition[p]==operator[CSG_UNION]) {
				glColor3f(.2,.5,.7);				
				primpos[0] = 1; primpos[1] = 0; primpos[2] = 0;
				primpos[4] = 0; primpos[5] = 1; primpos[6] = 0;
				primpos[8] = 0; primpos[9] = 0; primpos[10] = 1;
				glLoadMatrixf(primpos);
				draw_primitive(prim_operator);
			} else if (proposition[p]==operator[CSG_AND]) {
				glColor3f(.2,.5,.7);				
				primpos[0] = -1; primpos[1] = 0; primpos[2] = 0;
				primpos[4] = 0; primpos[5] = -1; primpos[6] = 0;
				primpos[8] = 0; primpos[9] = 0; primpos[10] = 1;
				glLoadMatrixf(primpos);
				draw_primitive(prim_operator);
			} else if (proposition[p]==operator[CSG_MINUS]) {
				glColor3f(.2,.5,.7);				
				primpos[0] = 1; primpos[1] = 0; primpos[2] = 0;
				primpos[4] = 0; primpos[5] = 0; primpos[6] = -1;
				primpos[8] = 0; primpos[9] = 1; primpos[10] = 0;
				glLoadMatrixf(primpos);
				draw_primitive(prim_operator);
			} else if (proposition[p]=='(') {
				glColor3f(.7,.3,.7);				
				primpos[0] = 0; primpos[1] = -1; primpos[2] = 0;
				primpos[4] = 1; primpos[5] = 0; primpos[6] = 0;
				primpos[8] = 0; primpos[9] = 0; primpos[10] = 1;
				glLoadMatrixf(primpos);
				draw_primitive(prim_operator);
			} else if (proposition[p]==')') {
				glColor3f(.7,.3,.7);				
				primpos[0] = 0; primpos[1] = 1; primpos[2] = 0;
				primpos[4] = -1; primpos[5] = 0; primpos[6] = 0;
				primpos[8] = 0; primpos[9] = 0; primpos[10] = 1;
				glLoadMatrixf(primpos);
				draw_primitive(prim_operator);
			} else {
				glColor3f(.7,.7,.7);				
				primpos[0] = camera.c[0];
				primpos[1] = camera.c[1];
				primpos[2] = camera.c[2];
				primpos[4] = camera.c[4];
				primpos[5] = camera.c[5];
				primpos[6] = camera.c[6];
				primpos[8] = camera.c[8];
				primpos[9] = camera.c[9];
				primpos[10] = camera.c[10];
				glLoadMatrixf(primpos);
				draw_mesh(e->bricks[proposition[p]-'a']);
			}
			if (p==paste_token-1) {
				primpos[12] += PRIM_SPACING*.2;
			} else if (p==paste_token) {
				primpos[12] -= PRIM_SPACING*.2;
			}
			primpos[12] += PRIM_SPACING;			
		}
		primpos[12] = PRIM_LEFT+PRIM_SPACING*((selection_start+selection_stop)*.5);
		primpos[0] = ((selection_stop+1-selection_start)*.5)*PRIM_SPACING; primpos[1] = 0; primpos[2] = 0;
		primpos[4] = 0; primpos[5] = 2; primpos[6] = 0;
		primpos[8] = 0; primpos[9] = 0; primpos[10] = 1;
		/* mesh_to_load[18] est le cube servant à la selection */
		glLoadMatrixf(primpos);
		glEnable(GL_BLEND);
		glColor4f(.2,.1,.5,.5);
		draw_mesh(mesh_to_load[18].m);
		glDisable(GL_BLEND);
		/* icons */
		{
			unsigned i_c;
			static double angle_flower=0;
			glDisable(GL_DEPTH_TEST);
			glViewport(0,0,glut_fenLong,h_pixel);
			glDisable(GL_LIGHTING);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
			primpos[0] = 1; primpos[1] = 0; primpos[2] = 0;
			primpos[4] = 0; primpos[5] = 1; primpos[6] = 0;
			primpos[8] = 0; primpos[9] = 0; primpos[10] = 1;
			primpos[12] = ICONS_START;
			primpos[14] = -ICONS_DEPTH;
			if (!rotation_step && !translation_step && !view_solution && !quit && !edit_session) icons[0].aff = 1;
			else icons[0].aff = 0;
			if (selection_start<selection_stop && !quit) icons[1].aff = 1;
			else icons[1].aff = 0;
			if (!view_solution && !quit && (timer[0]>0 || timer[1]>=3)) icons[2].aff = 1;
			else icons[2].aff = 0;
			if (!quit && !edit_session && !view_solution) icons[3].aff = 1;	/* only if not in edit mode */
			else icons[3].aff = 0;
			if (edit_session) icons[4].aff = 1;
			else icons[4].aff = 0;
			if (!quit && !edit_session && view_solution) icons[5].aff = 1;
			else icons[5].aff = 0;
			i_c = icon_clicked(glut_mouse_x, glut_mouse_y);
			for (u=0; u<sizeof(icons)/sizeof(*icons); u++) {
				if (icons[u].aff) {
					if (i_c == u) {
						primpos[14] = -ICONS_DEPTH*.99;	/* TODO rendre grace aux icones */
					} else {
						primpos[14] = -ICONS_DEPTH;
					}
					glLoadMatrixf(primpos);
					draw_mesh(mesh_to_load[icons[u].mesh_idx].m);
					primpos[12] -= ICONS_SPACING;//PRIM_SPACING;
				}
			}
			if (!edit_session && e->score>0) {
				/* time at left bottom */
				double diameter = .03*sin(angle_flower);
				if (timer[0]==0 && timer[1]<3) diameter *= 10;
				geom_set_identity(primpos);
				primpos[12] = -6.5;
				primpos[14] = -8.;
				glLoadMatrixf(primpos);
				glEnable(GL_TEXTURE_2D);
				glBindTexture(GL_TEXTURE_2D, flower[the_flower].tex->binding);
				put_char(26+timer[0], -.5, .7, .34+diameter);	/* 26 is '0' */
				primpos[12] += 1;
				glLoadMatrixf(primpos);
				put_char(46, -.5, .7, .34+diameter);	/* ':' */
				primpos[12] += 1;
				glLoadMatrixf(primpos);
				put_char(26+timer[1], -.5, .7, .34+diameter);
				primpos[12] += 1;
				glLoadMatrixf(primpos);
				put_char(26+timer[2], -.5, .7, .34+diameter);
				angle_flower += .1;
			}
			glDisable(GL_TEXTURE_2D);
			glDisable(GL_BLEND);
		}
		/* reset camera */
		glViewport(0,0,glut_fenLong,glut_fenHaut);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glFrustum(-1,1, -1,1, 1,100);
		glMatrixMode(GL_MODELVIEW);
	}
	{	/* timer */
		if (!quit && !edit_session && e->score>0) {
			if (utimer==0) {
				utimer=glut_interval;
				if (timer[2]==0) {
					timer[2] = 9;
					if (timer[1]==0) {
						timer[1] = 5;
						if (timer[0]==0) {
							timer[0] = timer[1] = timer[2] = 0;
							quit = 1;
							visio_perturb = .001;
							wana_view_solution = 0;
							sound_sample_start(6, 250, 1, 200);
							if (user_score>e->score) user_score --;
							else user_score = 1;
						} else timer[0]--;
					} else timer[1]--;
				} else timer[2]--;
			} else utimer--;
		}
	}
	if (quit && visio_perturb>1) {
		init = 0;
		if (edit_session) {
			for (u=0; u<e->nb_bricks; u++) {
				memcpy(e->solution_pos+u, e->bricks[u]->pos->c, sizeof(*e->solution_pos));
			}
			data_enigm_finalize(e, proposition);	/* simply removes unused prims and localize datas */
			data_enigm_save(e);
			if (~0==enigm_to_play) data_enigm_add(e);	/* to the list of known enigms */
		}
		if (csg_proposition) {
			csg_node_del(csg_proposition);	/* clean up for if we don't come back */
			csg_proposition = NULL;
		}
		mode_change(MODE_INTRO);	/* back to intro */
	}
}

void play_clic(int button, int pressed) {
	if (1==button) {
		if (pressed) {
			drag_stop_x = drag_start_x = glut_mouse_x;
			drag_stop_y = drag_start_y = glut_mouse_y;
			drag_new = 0;
			drag_on = 1;
			return;
		}
		drag_stop_x = glut_mouse_x;
		drag_stop_y = glut_mouse_y;
		drag_on = 0;
		drag_new = 1;
	} else {
		if (pressed) {
			paste_x = glut_mouse_x;
			paste_y = glut_mouse_y;
			paste_new = 1;
		}
	}
}

void play_motion(float x, float y) {
	if (drag_on) {
		if (glut_mouse_y>-.6) {
			double delta_horiz, delta_vert;
			delta_horiz = (glut_mouse_x - drag_stop_x)*2.;
			delta_vert = (glut_mouse_y - drag_stop_y)*2.;
			angle_horiz -= delta_horiz;
			angle_vert += delta_vert;
			if (angle_vert>M_PI*.5)
				angle_vert = M_PI*.5;
			else if (angle_vert<-M_PI*.5)
				angle_vert = -M_PI*.5;
			if (fabs(drag_stop_x-drag_start_x)+fabs(drag_stop_y-drag_start_y)>.1) camera_rotation_on = 1;
		}
		drag_stop_x = glut_mouse_x;
		drag_stop_y = glut_mouse_y;
	}
}

