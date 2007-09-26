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
#include <math.h>
#include <string.h>
#include "geom.h"
#include "csg.h"
#include "opengl.h"
#include "main.h"
#include "log.h"
#include "memspool.h"
#include "system.h"

#ifndef M_PI
#define M_PI       3.14159265358979323846
#endif

void geom_b_box_union(b_box *dest_bb, b_box *bb1, b_box *bb2)
{
	int i;
	/* resulting b_box encloses both */
	if (bb1->vide) {
		if (bb2->vide) {
			dest_bb->vide=1;
			return;
		} else {
			memcpy(dest_bb, bb2, sizeof(*dest_bb));
			return;
		}
	}
	if (bb2->vide) {
		memcpy(dest_bb, bb1, sizeof(*dest_bb));
		return;
	}
	memcpy(dest_bb, bb1, sizeof(*dest_bb));
	for (i=0; i<3; i++) {
		if (dest_bb->p_min[i]>bb2->p_min[i]) dest_bb->p_min[i] = bb2->p_min[i];
		if (dest_bb->p_max[i]<bb2->p_max[i]) dest_bb->p_max[i] = bb2->p_max[i];
	}
}

void geom_b_box_intersection(b_box *dest_bb, b_box *bb1, b_box *bb2)
{
	int i;
	/* resulting b_box is the intersection of both */
	if (bb1->vide || bb2->vide) {
		dest_bb->vide = 1;
		return;
	}
	for (i=0; i<3; i++) {
		if (bb1->p_min[i]>bb2->p_min[i]) {
			dest_bb->p_min[i] = bb1->p_min[i];
		} else {
			dest_bb->p_min[i] = bb2->p_min[i];
		}
		if (bb1->p_max[i]<bb2->p_max[i]) {
			dest_bb->p_max[i] = bb1->p_max[i];
		} else {
			dest_bb->p_max[i] = bb2->p_max[i];
		}
		if (dest_bb->p_min[i]>dest_bb->p_max[i]) {
			dest_bb->vide = 1;
			return;
		}
	}
	dest_bb->vide = 0;
}

void geom_get_b_box(csg_node *node, b_box *dest_bb)
{
	GLfloat pos[16];
	GLfloat size;
	GLfloat corner[7];
	unsigned i, j;
//	assert(node->type==CSG_PRIM);
	if (node->type!=CSG_PRIM) {
		csg_print_tree(node);
		gltv_log_fatal("Assertion node->type==CSG_PRIM failed. type=%d on :", node->type);
	}
	memcpy(pos, node->u.prim.m->pos->c, sizeof(pos));
	size = node->u.prim.m->prim->size_x;
	pos[0] *= size;
	pos[1] *= size;
	pos[2] *= size;
	size = node->u.prim.m->prim->size_y;
	pos[4] *= size;
	pos[5] *= size;
	pos[6] *= size;
	size = node->u.prim.m->prim->size_z;
	pos[8] *= size;
	pos[9] *= size;
	pos[10] *= size;
	for (i=0; i<3; i++) {
		dest_bb->p_min[i] = -pos[0+i] -pos[4+i] -pos[8+i] +pos[12+i];
		dest_bb->p_max[i] = dest_bb->p_min[i];
	}
	for (i=0; i<3; i++) {
		corner[0] = +pos[0+i] -pos[4+i] -pos[8+i] +pos[12+i];
		corner[1] = +pos[0+i] +pos[4+i] -pos[8+i] +pos[12+i];
		corner[2] = -pos[0+i] +pos[4+i] -pos[8+i] +pos[12+i];
		corner[3] = -pos[0+i] -pos[4+i] +pos[8+i] +pos[12+i];
		corner[4] = +pos[0+i] -pos[4+i] +pos[8+i] +pos[12+i];
		corner[5] = +pos[0+i] +pos[4+i] +pos[8+i] +pos[12+i];
		corner[6] = -pos[0+i] +pos[4+i] +pos[8+i] +pos[12+i];
		for (j=0; j<7; j++) {
			if (dest_bb->p_min[i]>corner[j]) dest_bb->p_min[i] = corner[j];
			if (dest_bb->p_max[i]<corner[j]) dest_bb->p_max[i] = corner[j];
		}
	}
	dest_bb->vide = 0;
}

GLfloat *current_modelview=NULL, *current_projection=NULL;

void geom_set_current_modelview(GLfloat *pos) 
{
	current_modelview = pos;
}
void geom_set_current_projection(GLfloat *proj)
{
	current_projection = proj;
}

void geom_matrix_mult(GLfloat *res, GLfloat *a, GLfloat *b) {
	unsigned i,j,k;
	/* the hurry man strikes back */
	for (i=0; i<4; i++) {
		for (j=0; j<4; j++) {
			res[(j<<2)+i] = 0;
			for (k=0; k<4; k++) {
				res[(j<<2)+i] += a[i+(k<<2)]*b[(j<<2)+k];
			}
		}
	}
}

void geom_matrix_transp_ortho(GLfloat *res, GLfloat *m)
{
	unsigned i,j;
	for (i=0; i<3; i++) {
		for (j=0; j<3; j++) {
			res[(i<<2)+j] = m[(j<<2)+i];
		}
		res[(i<<2)+3] = 0.;
	}
	for (i=0; i<3; i++) {
		res[12+i] = 0.;
		for (j=0; j<3; j++) {
			res[12+i] -= m[12+i]*res[(j<<2)+i];
		}
	}
	res[15] = 1;
}

void geom_set_identity(GLfloat *mat)
{
	unsigned i,j;
	for (i=0; i<4; i++) {
		for (j=0; j<4; j++) {
			mat[(i<<2)+j] = (i==j?1:0);
		}
	}
}

int geom_get_2d(GLfloat x, GLfloat y, GLfloat z, GLdouble *x2d, GLdouble *y2d, GLdouble *z2d)
{
	/* computes projection of a 3d point given current modelview and projection - returns 0 on error */
	/* the method of the hurry man : convert to double and use gluProject */
	GLint viewport[4] = { 0,0 };
	GLdouble modelview[16];
	GLdouble projection[16];
	unsigned i;
	assert(current_modelview!=NULL && current_projection!=NULL);
	viewport[2] = glut_fenLong;
	viewport[3] = glut_fenHaut;
	for (i=0; i<16; i++) {
		modelview[i] = current_modelview[i];
		projection[i] = current_projection[i];
	}
	return GL_TRUE == gluProject(x,y,z, modelview, projection, viewport, x2d,y2d,z2d);
}

void geom_get_b_box_2d(b_box *box3d, b_box_2d *box2d)
{
	/* computes the 2d bounding box from a 3d bounding box, the current camera and current projection */
	GLdouble x[8], y[8], z;
	unsigned i;
	if (box3d->vide) {
		box2d->vide=1;
		return;
	}
	i = geom_get_2d(box3d->p_min[0], box3d->p_min[1], box3d->p_min[2], &x[0], &y[0], &z);
	i += geom_get_2d(box3d->p_min[0], box3d->p_min[1], box3d->p_max[2], &x[1], &y[1], &z);
	i += geom_get_2d(box3d->p_min[0], box3d->p_max[1], box3d->p_min[2], &x[2], &y[2], &z);
	i += geom_get_2d(box3d->p_min[0], box3d->p_max[1], box3d->p_max[2], &x[3], &y[3], &z);
	i += geom_get_2d(box3d->p_max[0], box3d->p_min[1], box3d->p_min[2], &x[4], &y[4], &z);
	i += geom_get_2d(box3d->p_max[0], box3d->p_min[1], box3d->p_max[2], &x[5], &y[5], &z);
	i += geom_get_2d(box3d->p_max[0], box3d->p_max[1], box3d->p_min[2], &x[6], &y[6], &z);
	i += geom_get_2d(box3d->p_max[0], box3d->p_max[1], box3d->p_max[2], &x[7], &y[7], &z);
	if (8!=i) gltv_log_fatal("geom_get_b_box_2_d: invalid coordinates");
	box2d->xmin = box2d->xmax = x[0];
	box2d->ymin = box2d->ymax = y[0];
	for (i=1; i<8; i++) {
		if (x[i]<box2d->xmin) box2d->xmin=x[i];
		if (y[i]<box2d->ymin) box2d->ymin=y[i];
		if (x[i]>box2d->xmax) box2d->xmax=x[i];
		if (y[i]>box2d->ymax) box2d->ymax=y[i];
	}
	if (box2d->xmax>=glut_fenLong) box2d->xmax = glut_fenLong;
	if (box2d->ymax>=glut_fenHaut) box2d->ymax = glut_fenHaut;
	if (box2d->xmin<=0) box2d->xmin=0;
	if (box2d->ymin<=0) box2d->ymin=0;
	if (box2d->xmin>=box2d->xmax || box2d->ymin>=box2d->ymax) {
		box2d->vide=1;
	} else {
		box2d->vide = 0;
	}
//	gltv_log_warning(GLTV_LOG_MUSTSEE, "box2d : %d %d - %d %d - %d\n", box2d->xmin, box2d->ymin, box2d->xmax, box2d->ymax, (int)box2d->vide);
}

int geom_b_box_intersects(b_box *bb1, b_box *bb2)
{
	/* tels wether 2 b_box have an intersection */
	b_box inter;
	geom_b_box_intersection(&inter, bb1, bb2);
	return !inter.vide;
}

void geom_rapproche(GLfloat *a, GLfloat *b, float ratio) {
	float d;
	unsigned i;
	for (i=0; i<3; i++) {
		d = (b[i] - a[i])*ratio;
		a[i] += d;
	}
}

float geom_scalaire(GLfloat *a, GLfloat *b)
{
	return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];
}

float geom_norme(GLfloat *a)
{
	return sqrt(geom_scalaire(a,a));
}
	
void geom_normalise(GLfloat *a)
{
	double n = geom_norme(a);
	if (0!=n) {
		a[0] /= n;
		a[1] /= n;
		a[2] /= n;
	}
}

void geom_orthogonalise(GLfloat *a, GLfloat *b)
{
	double d = geom_scalaire(a,b);
	if (0!=d) {
		d /= geom_scalaire(b,b);
		a[0] -= d*b[0];
		a[1] -= d*b[1];
		a[2] -= d*b[2];
	}
}

void geom_vectoriel(GLfloat *a, GLfloat *b, GLfloat *c)
{
	c[0] = a[1]*b[2]-a[2]*b[1];
	c[1] = a[2]*b[0]-a[0]*b[2];
	c[2] = a[0]*b[1]-a[1]*b[0];
}

void geom_position_approche(GLfloat *now, GLfloat *target, float ratio)
{
	/* translation */
	geom_rapproche(now+12, target+12, ratio);
	/* axis */
	/* begin with z */
	geom_rapproche(now+8, target+8, ratio);
	geom_rapproche(now+4, target+4, ratio);
	geom_normalise(now+4);
	geom_orthogonalise(now+8, now+4);
	geom_normalise(now+8);
	geom_vectoriel(now+4, now+8, now+0);
}

int  geom_is_Pt_in_2DTriangle(struct Point2D * P, struct Point2D * p0, struct Point2D * p1, struct Point2D * p2)
{
	struct Point2D V[4];
	int i=0,  cn = 0;	// the crossing number counter

	V[i++] =* p0;
	V[i++] =* p1;
	V[i++] =* p2;
	V[i++] =* p0;
	// loop through all edges of the polygon
	for (i=0; i<3; i++) {	// edge from V[i] to V[i+1]
	   if (((V[i].y <= P->y) && (V[i+1].y > P->y))	// an upward crossing
		|| ((V[i].y > P->y) && (V[i+1].y <= P->y))) { // a downward crossing
			// compute the actual edge-ray intersect x-coordinate
			float vt = (float)(P->y - V[i].y) / (V[i+1].y - V[i].y);
			if (P->x < V[i].x + vt * (V[i+1].x - V[i].x)) // P->x < intersect
				++cn;   // a valid crossing of y=P->y right of P->x
		}
	}
	return (cn&1);	// 0 if even (out), and 1 if odd (in)
}

int geom_get_3d(GLdouble x, GLdouble y, GLdouble z, GLdouble *x3d, GLdouble *y3d, GLdouble *z3d)
{
	GLint viewport[4] = { 0,0 };
	GLdouble modelview[16];
	GLdouble projection[16];
	unsigned i;
	assert(current_modelview!=NULL && current_projection!=NULL);
	viewport[2] = glut_fenLong;
	viewport[3] = glut_fenHaut;
	for (i=0; i<16; i++) {
		modelview[i] = current_modelview[i];
		projection[i] = current_projection[i];
	}
	return GL_TRUE == gluUnProject(x,y,z, modelview, projection, viewport, x3d,y3d,z3d);
}

int geom_prim_clicked(primitive *prim)
{
	unsigned cptPts, cptPoly, nbPts, nbFaces;
	GLfloat* points;
	GLuint* faces;
	GLdouble dummy;
	int o, ret=0;
	struct Point2D *pts2D, p_mouseClickCoordinates;
	p_mouseClickCoordinates.x = (glut_mouse_x+1)*.5*glut_fenLong;
	p_mouseClickCoordinates.y = (1-glut_mouse_y)*.5*glut_fenHaut;
	if (prim->nb_points_light) {
		nbPts = prim->nb_points_light;
		nbFaces = prim->nb_faces_light;
		points = prim->points_light;
		faces = prim->faces_light;
	} else {
		nbPts = prim->nb_points;
		nbFaces = prim->nb_faces;
		points = prim->points;
		faces = prim->faces;
	}
	pts2D = gltv_memspool_alloc(sizeof(*pts2D)*nbPts);
	for (o=0, cptPts=0; cptPts<nbPts; cptPts++, o+=3) {
		geom_get_2d(points[o],points[o+1],points[o+2], &pts2D[cptPts].x,&pts2D[cptPts].y,&dummy);
	}
/*	glBegin(GL_LINES);
	glColor3f(1,1,0);
	glVertex3f(0, 0, -1.0001);
	glVertex3f(p_mouseClickCoordinates.x, p_mouseClickCoordinates.y, -1.0001);
	glEnd();*/
	for (cptPoly=0; cptPoly<nbFaces; cptPoly++) {
/*		glBegin(GL_LINES);
		glColor3f(1,0,1);
		glVertex3f(pts2D[faces[cptPoly*3+0]].x, pts2D[faces[cptPoly*3+0]].y, -1.0001);
		glVertex3f(pts2D[faces[cptPoly*3+1]].x, pts2D[faces[cptPoly*3+1]].y, -1.0001);
		glVertex3f(pts2D[faces[cptPoly*3+0]].x, pts2D[faces[cptPoly*3+0]].y, -1.0001);
		glVertex3f(pts2D[faces[cptPoly*3+2]].x, pts2D[faces[cptPoly*3+2]].y, -1.0001);
		glEnd();*/
		if (geom_is_Pt_in_2DTriangle(&p_mouseClickCoordinates, &pts2D[faces[cptPoly*3+0]],&pts2D[faces[cptPoly*3+1]],&pts2D[faces[cptPoly*3+2]])) {
			ret = 1;
			break;
		}
	}
	gltv_memspool_unregister(pts2D);
	return ret;
}

GLfloat geom_prim_depth(primitive *prim)
{
	/* as center is at (0,0,0), easy calculus (check that prims are still centered) */
	return current_modelview[14]; /* :-p */
}

mesh *geom_clic_position(position *pos, GLfloat *z)
{
	/* go throught all meshes of this position, computing
	 * the position of every meshes, and look for a clicked mesh.
	 * Return NULL if none found, or mesh
	 */
	GLfloat m[16], zz, *old_current;
	unsigned s;
	mesh *clicked = NULL, *oo;
	geom_matrix_mult(m, current_modelview, pos->c);
	old_current = current_modelview;
	geom_set_current_modelview(m);
/*	glPushMatrix();
	glLoadIdentity();*/
	for (s=0; s<pos->nb_sons; s++) {
		oo = geom_clic_position(pos->sons[s], &zz);
		if (oo!=NULL && (clicked==NULL || zz>*z)) {
			clicked = oo;
			*z = zz;
		}
	}
	for (s=0; s<pos->nb_meshes; s++) {
		if (geom_prim_clicked(pos->meshes[s]->prim)) {
			zz = geom_prim_depth(pos->meshes[s]->prim);
			if (clicked==NULL || zz>*z) {
				clicked = pos->meshes[s];
				*z = zz;
			}
		}
	}

	geom_set_current_modelview(old_current);
//	glPopMatrix();
	return clicked;
}

static void geom_get_pos_diff(GLfloat *dest, GLfloat *m1, GLfloat *m2)
{
	static GLfloat m1T[16];
	geom_matrix_transp_ortho(m1T, m1);
	geom_matrix_mult(dest, m1T, m2);
}

static int geom_axes_match(GLfloat *v1, GLfloat *v2)
{
	unsigned i;
	gltv_log_warning(GLTV_LOG_DEBUG, "\t\t\tdo (%f,%f,%f) matches (%f,%f,%f)?", v1[0],v1[1],v1[2],v2[0],v2[1],v2[2]);
	for (i=0; i<3; i++) {
		if (fabs(v1[i]-v2[i])>.1) return 0;
	}
	return 1;
}

static int geom_axes_unsigmatch(GLfloat *v1, GLfloat *v2)
{
	unsigned i;
	gltv_log_warning(GLTV_LOG_DEBUG, "\t\t\tdo (%f,%f,%f) matches +/-(%f,%f,%f)?", v1[0],v1[1],v1[2],v2[0],v2[1],v2[2]);
	for (i=0; i<3; i++) {
		if (fabs(fabs(v1[i])-fabs(v2[i]))>.1) return 0;
	}
	for (i=0; i<3; i++) {
		if (fabs(v1[i]+v2[i])>.1) return 1;
	}
	return -1;
}

static int geom_pos_are_equivalent(GLfloat *m1, GLfloat *m2, unsigned sym2)
{
	/* Ts should be equals */
	gltv_log_warning(GLTV_LOG_DEBUG, "\t\tT match ?");
	if (!geom_axes_match(m1+12,m2+12)) {
		gltv_log_warning(GLTV_LOG_DEBUG, "\t\tno");
		return 0;
	}
	gltv_log_warning(GLTV_LOG_DEBUG, "\t\tyes\n\t\tsym = %u", sym2);
	/* We dont handle every possible symmetry because we lack the time */
	if (0==sym2) {
		return geom_axes_match(m1, m2) && geom_axes_match(m1+4, m2+4) && geom_axes_match(m1+8, m2+8);
	} else if (ROT_X_45+ROT_Y_45+ROT_Z_45==sym2) {
		return 1;
	} else if (ROT_X_45==sym2) {
		return geom_axes_match(m1, m2);
	} else if (ROT_Y_45==sym2) {
		return geom_axes_match(m1+4, m2+4);
	} else if (ROT_Z_45==sym2) {
		return geom_axes_match(m1+8, m2+8);
	} else if (ROT_X_45+ROT_Y_90+ROT_Z_90==sym2) {
		return geom_axes_unsigmatch(m1, m2);
	} else if (ROT_X_90+ROT_Y_45+ROT_Z_90==sym2) {
		return geom_axes_unsigmatch(m1+4, m2+4);
	} else if (ROT_X_90+ROT_Y_90+ROT_Z_45==sym2) {
		return geom_axes_unsigmatch(m1+8, m2+8);
	} else if (ROT_X_90==sym2) {
		return geom_axes_match(m1, m2) && (geom_axes_unsigmatch(m1+4,m2+4)*geom_axes_unsigmatch(m1+8,m2+8))==1;
	} else if (ROT_Y_90==sym2) {
		return geom_axes_match(m1+4, m2+4) && (geom_axes_unsigmatch(m1,m2)*geom_axes_unsigmatch(m1+8,m2+8))==1;
	} else if (ROT_Z_90==sym2) {
		return geom_axes_match(m1+8, m2+8) && (geom_axes_unsigmatch(m1+4,m2+4)*geom_axes_unsigmatch(m1,m2))==1;
	} else {
		gltv_log_fatal("symmetry not handled for now : %u", sym2);
	}
	return 0;	// clears a warning. gltv_log_fatal does not return.
}

int geom_pos_is_equivalent(mesh **m, GLfloat (*pos)[16], unsigned nb_pos)
{
	static GLfloat diff[2][16];
	unsigned p;
	GLfloat s[2];
	char k[2];
	static GLfloat posInit[16];
	if (nb_pos<2) return 1;
	posInit[12] = m[0]->pos->c[12];
	posInit[13] = m[0]->pos->c[13];
	posInit[14] = m[0]->pos->c[14];
	posInit[15] = 1.;
	gltv_log_warning(GLTV_LOG_DEBUG, "Are positions equivalents ?");
	/* TODO : au lieu de prendre le premier mesh comme référence, prendre celui qui à le moins de symetrie ... imagine que ce soit une sphere le premier mesh ! */
	for (s[0]=-1; s[0]<=1; s[0]+=2) {
		for (s[1]=-1; s[1]<=1; s[1]+=2) {
			for (k[0]=0; k[0]<3; k[0]++) {
				for (k[1]=0; k[1]<3; k[1]++) {
					unsigned i,j;
					if (k[0]==k[1]) continue;
					for (i=0; i<2; i++) {
						for (j=0; j<3; j++) {
							if (k[i]==j) posInit[(i<<2)+j] = s[i];
							else posInit[(i<<2)+j] = 0;
						}
						geom_vectoriel(posInit, posInit+4, posInit+8);
						if (geom_pos_are_equivalent(m[0]->pos->c, posInit, m[0]->prim->symmetry)) {
							for (p=0; p<nb_pos; p++) {
								geom_get_pos_diff(diff[0], posInit, m[p]->pos->c);
								geom_get_pos_diff(diff[1], &pos[0][0], &pos[p][0]);
								gltv_log_warning(GLTV_LOG_DEBUG, "\tpositions of prim %u for prim 0 equivalents ?", p);
								if (!geom_pos_are_equivalent(diff[0], diff[1], m[p]->prim->symmetry)) {
									gltv_log_warning(GLTV_LOG_DEBUG, "\tno");
									break;
								}
								gltv_log_warning(GLTV_LOG_DEBUG, "\tyes");
							}	
							if (p==nb_pos) return 1;
						}
					}
				}
			}
		}
	}
	return 0;
}
