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
#ifndef GEOM_H
#define GEOM_H

#include <math.h>
#include "opengl.h"
#include "csg.h"
#include "data.h"

struct Point2D { GLdouble x, y; };
struct face 	{ 		unsigned idxP0,idxP1,idxP2; 	};

extern void geom_b_box_union(b_box *, b_box *, b_box *);
extern void geom_b_box_intersection(b_box *, b_box *, b_box *);
extern void geom_get_b_box(csg_node *, b_box *);
extern void geom_get_b_box_2d(b_box *, b_box_2d *);
extern int geom_b_box_intersects(b_box *, b_box *);
extern void geom_set_current_modelview(GLfloat*);
extern void geom_set_current_projection(GLfloat*);

extern void geom_rapproche(GLfloat *, GLfloat *, float);
extern float geom_scalaire(GLfloat *, GLfloat *);
extern float geom_norme(GLfloat *);
extern void geom_normalise(GLfloat *);
extern void geom_orthogonalise(GLfloat *, GLfloat *);
extern void geom_vectoriel(GLfloat *, GLfloat *, GLfloat *);
extern void geom_position_approche(GLfloat *, GLfloat *, float);
extern int geom_get_3d(GLdouble, GLdouble, GLdouble, GLdouble *, GLdouble *, GLdouble *);
extern void geom_set_identity(GLfloat *);
extern void geom_matrix_mult(GLfloat *, GLfloat *, GLfloat *);

extern mesh *geom_clic_position(position *, GLfloat *);
extern int geom_pos_is_equivalent(mesh **, GLfloat (*)[16], unsigned);

#endif
