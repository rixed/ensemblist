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
#ifndef DRAW_H
#define DRAW_H

#include "csg.h"
#include "data.h"

extern int with_ktx, with_wgl;

extern void draw_init();	/* to be called once opengl is ready and before call to draw functions */
extern void draw_csg_tree(csg_node *);
extern void draw_union_of_products(csg_union_of_products *);
extern void draw_union_of_partial_products(csg_union_of_partial_products *);
extern void draw_mesh(mesh *);
extern void draw_position(position *);
extern void draw_primitive(primitive *);

#endif
