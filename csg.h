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
#ifndef CSG_H
#define CSG_H

#include "data.h"

typedef enum { CSG_STRING, CSG_PRIM, CSG_OP } csg_node_type;
typedef enum { CSG_NOOP, CSG_MINUS, CSG_UNION, CSG_AND } csg_op;

typedef struct {
	char vide;
	GLfloat p_min[3];
	GLfloat p_max[3];
} b_box;

typedef struct {
	char vide;
	GLint xmin, ymin;
	GLint xmax, ymax;
} b_box_2d;

/* we use primitives rather than mesh for the enigms, because we
 * have a special positionning system, and we don't care about
 * rendering informations of a mesh */
struct csg_node_s {
	csg_node_type type;
	union {
		struct {
			char *string;
			unsigned len;
		} s;
		struct {
			mesh *m;
			b_box box3d;
			b_box_2d box2d;
		} prim;
		struct {
			csg_op op;
			struct csg_node_s *left, *right;
		} op;
	} u;
};
typedef struct csg_node_s csg_node;

#define NB_MAX_PRIMS_IN_TREE 16

typedef struct {
	unsigned nb_unions;
	struct {
		unsigned nb_products;
		struct {
			csg_node *node;
			csg_op op_to_next;
		} products[NB_MAX_PRIMS_IN_TREE];
	} unions[NB_MAX_PRIMS_IN_TREE];
} csg_union_of_products;

/* A partial product is an array of primitive, the first
 * beeing the target and the other the trimmers (primitive against which the
 * target is trimmed). Beeing frontal, for a trimmer, means that target
 * intersects the negation of this primitive ; and for the target, beeing frontal
 * means targeting the front facing faces, back facing otherwise.
 * The union_of_product must be converted to a union of partial products before
 * rendering.
 */

typedef struct {
	unsigned nb_products;
	unsigned group_id;
	struct {
		csg_node *node;
		char frontal;
	} products[NB_MAX_PRIMS_IN_TREE];
} csg_trimmed_target;	/* the first product is the target */

typedef struct {
	unsigned nb_unions;
	csg_trimmed_target unions[NB_MAX_PRIMS_IN_TREE*NB_MAX_PRIMS_IN_TREE];
	b_box_2d max2d_group[NB_MAX_PRIMS_IN_TREE];	/* max 2d region of a group */
} csg_union_of_partial_products;

/* ascii representation of operators */
extern const char operator[];	/* c++ compilos wont like this one :) */

/* build a csg tree from a string, with the primitives given, or NULL if error */
extern csg_node *csg_build_tree(char *, unsigned, mesh **);
/* deletes (recursively) a node - then, a tree - */
extern void csg_node_del(csg_node *); 
/* for debug purpose, print the csg tree on stdout */
extern void csg_print_tree(csg_node *);
/* computes the flat version of a csg tree : a union of products */
extern csg_union_of_products *csg_union_of_products_new(csg_node *);
/* rebuilds the uop */
extern int csg_union_of_products_reset(csg_union_of_products *, csg_node *);
/* deletes it */
extern void csg_union_of_products_del(csg_union_of_products *);
/* for debug purpose, prints it */
extern void csg_union_of_products_print(csg_union_of_products *);
/* get bounding box of a csg */
void csg_get_b_box(csg_node *, b_box *);
/* computes the union of partial products */
extern csg_union_of_partial_products *csg_union_of_partial_products_new(csg_union_of_products *);
/* rebuilds the uopp */
extern int csg_union_of_partial_products_reset(csg_union_of_partial_products *, csg_union_of_products *);
/* deletes it */
extern void csg_union_of_partial_products_del(csg_union_of_partial_products *);
/* for debug purpose, prints it */
extern void csg_union_of_partial_products_print(csg_union_of_partial_products *);
/* compute the bounding boxs (3d and 2d) for the csg once the camera and projection have been set up */
extern void csg_union_of_partial_products_resize(csg_union_of_partial_products *);
/* tells wether 2 csg are the same (without considering positions) */
extern int csg_is_equivalent(csg_union_of_partial_products *, csg_union_of_partial_products *);

#endif
