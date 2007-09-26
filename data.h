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
#ifndef DATA_H
#define DATA_H
#include "opengl.h"
#include "hash.h"
#include "slist.h"
#include "system.h"

/* position */

#define NB_MAX_SUBPOSITION 30 //6	/* TODO : fix root so that its no more used nor loaded */
#define NB_MAX_MESHES_IN_POSITION 16
struct mesh_s;

struct position_s {
	GLfloat c[16];
	struct position_s *relative_to;
	unsigned nb_sons;
	char const *name;
	unsigned nb_meshes;
	struct mesh_s *meshes[NB_MAX_MESHES_IN_POSITION];
	struct position_s *sons[NB_MAX_SUBPOSITION];
};
typedef struct position_s position;

/* primitive */

#define ROT_X_45 1
#define ROT_X_90 2
#define ROT_Y_45 4
#define ROT_Y_90 8
#define ROT_Z_45 16
#define ROT_Z_90 32

/* meshes */

typedef struct {
	char *name;
	GLuint binding;
} texture;

struct primitive_s {
	unsigned nb_points, nb_faces;
	GLfloat *points, *norms;
	GLuint *faces;
	unsigned nb_points_light, nb_faces_light;
	GLfloat *points_light, *norms_light;
	GLuint *faces_light;
	unsigned symmetry;	/* combinaison of ROTs */
	float size_x, size_y, size_z;
	char *name;
};
typedef struct primitive_s primitive;
extern GLTV_HASH enigm_meshes;	/* meshes usables for csg */

typedef struct {
	GLfloat *c;
	char *name;
} uv_coords;

/* severall instance of a mesh can be created, in wich case coord_uv, prims, name & texture, are merged, and position cleared */
struct mesh_s {
	texture *texture;
	primitive *prim;	/* c'est là dessus que se fait la gestion d'evenements */
	uv_coords *uv_coord;
	position *pos;
	char *name;
};
typedef struct mesh_s mesh;

/* enigms */

typedef struct {
	unsigned nb_bricks;
	mesh **bricks;
	char *solution;
	GLfloat (*solution_pos)[16];
	unsigned score;
	char *name;
	struct csg_node_s *root;
} enigm;

extern GLTV_SLIST enigms, editable_enigms;	/* sorted by score */

/* calls */

extern void data_read_all();
extern void data_read_userland();
extern texture *data_load_texture(const char *);
extern mesh *data_load_mesh(const char *, int);
extern void data_bind_mesh(position *, mesh *);
extern position *data_get_named_position(const char *);
extern primitive *data_load_primitive(const char *, int);
extern enigm *data_enigm_full_new(char *);
extern void data_enigm_finalize(enigm *, char *);
extern void data_enigm_save(enigm *);
extern void data_enigm_add(enigm *);
extern void data_enigm_del(enigm *);
extern enigm *data_load_enigm(const char *);

#endif
