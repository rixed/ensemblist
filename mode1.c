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
#include "draw.h"
#include "modes.h"
#include "slist.h"
#include "log.h"
#include "opengl.h"
#include "geom.h"

/* DEBUG : affiche la CSG (1?re ?nigme) et modifie les objets */

static GLfloat camera[16] = {
	1,0,0,0,
	0,1,0,0,
	0,0,1,0,
	0,-2,-6,1	/* decaler en X sur la gauche, vent vers la droite */
};
static GLfloat projection[16];
static double angle=0;

static mesh *test_mesh;

void m1_display(void) {
	static csg_union_of_partial_products *uopp = NULL;
	double ca, sa;
	if (NULL==uopp) {
		csg_union_of_products *uop;
		enigm *e = gltv_slist_get(enigms, 0);
		uop = csg_union_of_products_new(e->root);
		if (uop) {
			uopp = csg_union_of_partial_products_new(uop);
		} else {
			gltv_log_fatal("m1_display: Cannot get union of products");
		}
		if (NULL==uopp) {
			gltv_log_fatal("m1_display: Cannot get union of partial products");
		}
		glClearColor(0.18, 0.28, 0.28, 0.0);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glFrustum(-1,1, -1,1, 1,1000);
		glMatrixMode(GL_MODELVIEW);
		glGetFloatv(GL_PROJECTION_MATRIX, projection);
		geom_set_current_projection(projection);
		test_mesh = data_load_mesh("num_cylinder.mesh", 0);
		if (NULL==test_mesh) gltv_log_fatal("mode1: Cannot load mesh");
		glColor3f(.2,.5,.5);
	}
	ca = cos(angle);
	sa = sin(angle);	
	camera[0] = ca;
	camera[2] = sa;
	camera[8] = -sa;
	camera[10] = ca;
	angle += .015;
	glStencilMask(~0);
	glDepthMask(1);
	glColorMask(1,1,1,1);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
	glLoadMatrixf(camera);
	geom_set_current_modelview(camera);
	draw_union_of_partial_products(uopp);
	// and a test_mesh
//	draw_mesh(test_mesh);
}
