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
#include <string.h>
#include <math.h>
#include <assert.h>
#include <errno.h>
#include "data.h"
#include "hash.h"
#include "slist.h"
#include "stack.h"
#include "memspool.h"
#include "csg.h"
#include "png2gl.h"
#include "opengl.h"
#include "log.h"
#include "geom.h"

GLTV_HASH enigm_primitives;	/* loaded primitives for enigms */
GLTV_SLIST enigms;
GLTV_SLIST editable_enigms;
static GLTV_HASH textures = NULL;

static void data_free_textures()
{
	unsigned dummy_key;
	texture *tex;
	gltv_hash_reset(textures);
	while (gltv_hash_each(textures, &dummy_key, (void**)&tex)) {
		gltv_memspool_unregister(tex->name);
		gltv_memspool_unregister(tex);
	}
	gltv_hash_del(textures);
}

texture *data_load_texture(const char *file_name)
{
	size_t len;
	static char png2gl_needs_init = 1;
	texture *tex;
	if (textures==NULL) {
		textures = gltv_hash_new(30, 2, GLTV_HASH_STRKEYS);
		if (!textures) {
			gltv_log_fatal("Cannot init hash for textures");
		}
		atexit(data_free_textures);
	}
	if (png2gl_needs_init) {
		png2gl_init();
		png2gl_needs_init = 0;
	}
	if (gltv_hash_get(textures, (unsigned)file_name, (void**)&tex)) {
		return tex;
	}
	tex = gltv_memspool_alloc(sizeof(*tex));
	if (!tex) {
		gltv_log_warning(GLTV_LOG_MUSTSEE, "data_load_texture: Cannot get memory");
		return NULL;
	}
	tex->binding = png2gl_load_image(file_name);
	glBindTexture(GL_TEXTURE_2D, tex->binding);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	/* basic config */
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	len = strlen(file_name)+1;
	tex->name = gltv_memspool_alloc(len);
	if (!tex->name) {
		gltv_memspool_unregister(tex);
		return NULL;
	}
	memcpy(tex->name, file_name, len);
	gltv_hash_put(textures, (unsigned)(tex->name), tex);
	return tex;
}

static int enigm_cmp(void *e1, void *e2)
{
	if (((enigm*)e1)->score<((enigm*)e2)->score) return -1;
	if (((enigm*)e1)->score>((enigm*)e2)->score) return 1;
	return 0;
}

static void bound_primitive(primitive *prim, int recentre)
{
	/* recentre la primitive et calcule la taille de sa boite englobante */
	double x=0, y=0, z=0, f;
	unsigned i;
	if (recentre) {
		for (i=0; i<prim->nb_points; i++) {
			x += prim->points[i*3+0];
			y += prim->points[i*3+1];
			z += prim->points[i*3+2];
		}
		x /= i;
		y /= i;
		z /= i;
	}
	prim->size_x = prim->size_y = prim->size_z = 0;
	for (i=0; i<prim->nb_points; i++) {
		if (recentre) {
			prim->points[i*3+0] -= x;
			prim->points[i*3+1] -= y;
			prim->points[i*3+2] -= z;
		}
		if ((f=fabs(prim->points[i*3+0]))>prim->size_x) prim->size_x = f;
		if ((f=fabs(prim->points[i*3+1]))>prim->size_y) prim->size_y = f;
		if ((f=fabs(prim->points[i*3+2]))>prim->size_z) prim->size_z = f;
	}
	if (recentre) {
		for (i=0; i<prim->nb_points_light; i++) {
			prim->points_light[i*3+0] -= x;
			prim->points_light[i*3+1] -= y;
			prim->points_light[i*3+2] -= z;
		}
	}
}

static GLTV_HASH primitives = NULL;

static void data_free_primitives()
{
	unsigned dummy_key;
	primitive *prim;
	gltv_hash_reset(primitives);
	while (gltv_hash_each(primitives, &dummy_key, (void**)&prim)) {
		gltv_memspool_unregister(prim->name);
		if (prim->points) gltv_memspool_unregister(prim->points);
		if (prim->norms) gltv_memspool_unregister(prim->norms);
		if (prim->faces) gltv_memspool_unregister(prim->faces);
		if (prim->points_light) gltv_memspool_unregister(prim->points_light);
		if (prim->norms_light) gltv_memspool_unregister(prim->norms_light);
		if (prim->faces_light) gltv_memspool_unregister(prim->faces_light);
		gltv_memspool_unregister(prim);
	}
	gltv_hash_del(primitives);
}

primitive *data_load_primitive(const char *file_name, int recentre)
{
#define LINE_LENGTH 256
	char line[LINE_LENGTH];
	char *name;
	FILE *file;
	size_t len = strlen(file_name);
	primitive *prim;
	unsigned nb_points[2], nb_faces[2], nb_norms[2];
	int faces_section=-1, points_section=-1, in_section=0;
	unsigned symmetry = 0;
	GLuint *faces[2];
	GLfloat *points[2];
	GLfloat *norms[2];
	unsigned nb_faces_max[2], nb_points_max[2];
	int i;
	if (NULL==primitives) {
		primitives = gltv_hash_new(30, 2, GLTV_HASH_STRKEYS);
		if (!primitives) {
			gltv_log_fatal("Cannot init hash for primitives");
		}
		atexit(data_free_primitives);
	}
	if (gltv_hash_get(primitives, (unsigned)file_name, (void**)&prim)) {
		return prim;
	}
	gltv_log_warning(GLTV_LOG_DEBUG, "Reading primitive from file '%s'\n", file_name);
	if (NULL==(file=fopen(file_name,"r"))) {
		gltv_log_warning(GLTV_LOG_MUSTSEE, "data_load_primitive: Cannot open %s because : %s", file_name, strerror(errno));		
		return NULL;
	}
	name = gltv_memspool_alloc(len+1);
	if (!name) {
mem_error:
		puts("Cannot get memory\n");
inner_quit:
		fclose(file);
		return NULL;
	}
	strncpy(name, file_name, len+1);
	prim = gltv_memspool_alloc(sizeof(*prim));
	if (!prim) goto mem_error;
	for (i=0; i<2; i++) {
		nb_faces_max[i] = 500;
		nb_faces[i] = 0;
		faces[i] = gltv_memspool_alloc(sizeof(*faces[0])*3*nb_faces_max[i]);
		if (!faces[i]) goto mem_error;
		nb_points_max[i] = 500;
		nb_points[i] = 0;
		points[i] = gltv_memspool_alloc(sizeof(*points[0])*3*nb_points_max[i]);
		if (!points[i]) goto mem_error;
		norms[i] = NULL;
	}
	while (NULL!=fgets(line, LINE_LENGTH, file)) {
		if (strlen(line)<2) continue;
		if (0==strncmp(line, "ROT_X_45", 8)) {
			symmetry |= ROT_X_45;
		} else if (0==strncasecmp(line, "ROT_X_90", 8)) {
			symmetry |= ROT_X_90;
		} else if (0==strncasecmp(line, "ROT_Y_45", 8)) {
			symmetry |= ROT_Y_45;
		} else if (0==strncasecmp(line, "ROT_Y_90", 8)) {
			symmetry |= ROT_Y_90;
		} else if (0==strncasecmp(line, "ROT_Z_45", 8)) {
			symmetry |= ROT_Z_45;
		} else if (0==strncasecmp(line, "ROT_Z_90", 8)) {
			symmetry |= ROT_Z_90;
		} else if (0==strncasecmp(line, "points", 6)) {
			points_section++;
			in_section = 0;
			if (points_section>=2) goto format_error;
		} else if (0==strncasecmp(line, "norms", 5)) {
			in_section = 1;
			if (points_section<0 || points_section>=2) goto format_error;
			nb_norms[points_section]=0;
			norms[points_section] = gltv_memspool_alloc(sizeof(*norms[0])*3*nb_points[points_section]);
			if (!norms[points_section]) goto mem_error;
		} else if (0==strncasecmp(line, "faces", 5)) {
			faces_section++;
			in_section = 2;
			if (faces_section>=2) goto format_error;
		} else if (in_section==2) {
			unsigned p[3];
			if (3!=sscanf(line, "%u %u %u", p+0, p+1, p+2)) {
format_error:
				gltv_log_fatal("Bad line : '%s'\n", line);
			}
			if (nb_faces_max[faces_section]==nb_faces[faces_section]) {
				void *temp = gltv_memspool_realloc(faces[faces_section], sizeof(*faces[0])*3*(nb_faces_max[faces_section]*=2));
				if (!temp) goto mem_error;
				faces[faces_section] = temp;
			}
			for (i=0; i<3; i++) faces[faces_section][nb_faces[faces_section]*3+i] = p[i];
			nb_faces[faces_section]++;
		} else if (in_section==0) {
			float p[3];
			if (3!=sscanf(line, "%f %f %f", p+0, p+1, p+2)) goto format_error;
			if (nb_points_max[points_section]==nb_points[points_section]) {
				void *temp = gltv_memspool_realloc(points[points_section], sizeof(*points[0])*3*(nb_points_max[points_section]*=2));
				if (!temp) goto mem_error;
				points[points_section] = temp;
			}
			for (i=0; i<3; i++) points[points_section][nb_points[points_section]*3+i] = p[i];
			nb_points[points_section]++;
		} else { // in_section==2
			float p[3];
			double n;
			if (3!=sscanf(line, "%f %f %f", p+0, p+1, p+2)) goto format_error;
			n = sqrt(p[0]*p[0]+p[1]*p[1]+p[2]*p[2]);
			if (n!=0) {
				p[0] /= n;
				p[1] /= n;
				p[2] /= n;
			}
			if (nb_norms[points_section]>=nb_points[points_section]) goto format_error;
			for (i=0; i<3; i++) norms[points_section][nb_norms[points_section]*3+i] = p[i];
			nb_norms[points_section]++;
		}
	}
	for (i=0; i<2; i++) {
		void *temp1, *temp2;
		temp1 = gltv_memspool_realloc(points[i], sizeof(*points[0])*3*nb_points[i]);
		temp2 = gltv_memspool_realloc(faces[i], sizeof(*faces[0])*3*nb_faces[i]);
		if (!temp1 || !temp2) {
			puts("Warning : cannot resize memory");
		} else {
			points[i] = temp1;
			faces[i] = temp2;
		}
	}
	if (nb_faces[0]>=nb_faces[1]) i=0;
	else i=1;
	if (-1==points_section || -1==faces_section || 0==nb_points[i] || 0==nb_faces[i]) {
		puts("Not enought geometry");
		goto inner_quit;
	}
	prim->symmetry = symmetry;
	prim->nb_points = nb_points[i];
	prim->nb_faces = nb_faces[i];
	prim->points = points[i];
	prim->norms = norms[i];
	prim->faces = faces[i];
	prim->name = name;
	i ^= 1;
	if (nb_points[i]>0 && nb_faces[i]>0) {
		prim->nb_points_light = nb_points[i];
		prim->nb_faces_light = nb_faces[i];
		prim->points_light = points[i];
		prim->norms_light = norms[i];
		prim->faces_light = faces[i];
	} else {
		prim->nb_points_light = 0;
		prim->nb_faces_light = 0;
		prim->points_light = NULL;
		prim->norms_light = NULL;
		prim->faces_light = NULL;
		gltv_memspool_unregister(points[i]);
		gltv_memspool_unregister(faces[i]);
	}
	bound_primitive(prim, recentre);
	fclose(file);
	gltv_hash_put(primitives, (unsigned)(prim->name), prim);
	return prim;
}

static GLTV_HASH positions = NULL;	/* can not be instanciated more than once */
position *data_get_named_position(const char *name)
{
	position *pos;
	assert(NULL!=positions);
	if (gltv_hash_get(positions, (unsigned)name, (void**)&pos)) {
		return pos;
	} else {
		return NULL;
	}
}

static void data_free_positions()
{
	unsigned dummy_key;
	position *pos;
	gltv_hash_reset(positions);
	while (gltv_hash_each(positions, &dummy_key, (void**)&pos)) {
		gltv_memspool_unregister(pos->name);
		gltv_memspool_unregister(pos);
	}
	gltv_hash_del(positions);
}

position *data_load_position(const char *file_name)
{	/* also loads all necessary datas - reentrant */
	FILE *file;
	char line[LINE_LENGTH];
	position *pos;
	size_t len;
	int i;
	if (NULL==positions) {
		positions = gltv_hash_new(30, 2, GLTV_HASH_STRKEYS);
		if (!positions) {
			positions=NULL;
			gltv_log_fatal("Cannot init hash for positions");
		}
		atexit(data_free_positions);
	}
	if (gltv_hash_get(positions, (unsigned)file_name, (void**)&pos)) {
		return pos;
	}
	pos = gltv_memspool_alloc(sizeof(*pos));
	if (!pos) {
		gltv_log_fatal("Cannot get memory for position");
	}
	gltv_log_warning(GLTV_LOG_OPTIONAL, "Reading position from file '%s'\n", file_name);
	if (NULL==(file=fopen(file_name,"r"))) {
		gltv_log_warning(GLTV_LOG_MUSTSEE, "data_load_position: Cannot open %s because : %s", file_name, strerror(errno));		
		gltv_memspool_unregister(pos);
		return NULL;
	}
	pos->relative_to = NULL;
	pos->nb_sons = 0;
	pos->nb_meshes = 0;
	pos->name = NULL;
	for (i=0; i<16; i++) {
		pos->c[i] = ((i>>2)==(i&3))?1:0;	/* diagonal */
	}
	while (NULL!=fgets(line, LINE_LENGTH, file)) {
		len = strlen(line);
		if (len<2) continue;
		line[--len] = '\0';
		if (0==strncasecmp(line, "relative", 8)) {
			if (NULL==fgets(line, LINE_LENGTH, file)) {
bad_pos:
				gltv_log_fatal("Bad position");
			}
			len = strlen(line);
			if (len<2) goto bad_pos;
			line[--len] = '\0';
			pos->relative_to = data_load_position(line);
			if (NULL==pos->relative_to) goto bad_pos;
			if (NB_MAX_SUBPOSITION == pos->relative_to->nb_sons) {
				gltv_log_fatal("load_position : position '%s' with too many children (while attempting to add '%s')", pos->relative_to->name, file_name);
			}
			pos->relative_to->sons[pos->relative_to->nb_sons++] = pos;
		} else if (0==strncasecmp(line, "position", 8)) {
			if (16!=fscanf(file, "%f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f",
						&pos->c[0], &pos->c[1], &pos->c[2], &pos->c[3],
						&pos->c[4], &pos->c[5], &pos->c[6], &pos->c[7],
						&pos->c[8], &pos->c[9], &pos->c[10], &pos->c[11],
						&pos->c[12], &pos->c[13], &pos->c[14], &pos->c[15])) goto bad_pos;
		} else {
			gltv_log_fatal("Bad syntax line '%s'", line);
		}
	}
	fclose(file);
/*	geom_normalise(pos->c+0);
	geom_normalise(pos->c+4);
	geom_normalise(pos->c+8);*/
	len = strlen(file_name)+1;
	pos->name = gltv_memspool_alloc(len);
	if (!pos->name) {
		gltv_log_fatal("load_position: Cannot get memory for name");
	}
	memcpy(pos->name, file_name, len);
	gltv_hash_put(positions, (unsigned)(pos->name), pos);
	return pos;
}

void data_bind_mesh(position *pos, mesh *m)
{
	assert(pos!=NULL);
	assert(m!=NULL);
	if (m->pos == pos) return;
	if (pos->nb_meshes>=NB_MAX_MESHES_IN_POSITION) {
		gltv_log_fatal("bind mesh '%s' to position '%s' : no room", m->name, pos->name);
	}
	if (NULL!=m->pos) {	/* unbind */
		unsigned i;
		position *pos2 = m->pos;
		for (i=0; i<pos2->nb_meshes; i++)
			if (pos2->meshes[i] == m) break;
		if (i==pos2->nb_meshes) gltv_log_fatal("cannot unbind mesh");
		pos2->nb_meshes--;
		for (; i<pos2->nb_meshes; i++) {
			pos2->meshes[i] = pos2->meshes[i+1];
		}
	}
	m->pos = pos;	/* rebind */
	pos->meshes[pos->nb_meshes++] = m;
}

static GLTV_HASH coords = NULL;

static void data_free_coords()
{
	unsigned dummy_key;
	uv_coords *uv;
	gltv_hash_reset(coords);
	while (gltv_hash_each(coords, &dummy_key, (void**)&uv)) {
		gltv_memspool_unregister(uv->name);
		gltv_memspool_unregister(uv->c);
		gltv_memspool_unregister(uv);
	}
	gltv_hash_del(coords);
}

uv_coords *data_load_uv_coords(const char *file_name, unsigned nb_pts)
{	/* cannot be instanciated more than once */
	FILE *file;
	uv_coords *uv;
	size_t len;
	unsigned p;
	if (NULL==coords) {
		coords = gltv_hash_new(30, 2, GLTV_HASH_STRKEYS);
		if (!coords) {
			gltv_log_fatal("Cannot init hash for meshes");
		}
		atexit(data_free_coords);
	}
	if (gltv_hash_get(coords, (unsigned)file_name, (void**)&uv)) {
		return uv;
	}
	uv = gltv_memspool_alloc(sizeof(*uv));
	if (!uv) {
uv_nomem:
		gltv_log_fatal("Cannot get memory for uv_coords");
	}
	uv->c = gltv_memspool_alloc(sizeof(*uv->c)*2*nb_pts);
	if (!uv->c) goto uv_nomem;
	gltv_log_warning(GLTV_LOG_DEBUG, "Reading uv_coords from file '%s'\n", file_name);
	if (NULL==(file=fopen(file_name,"r"))) {
		gltv_log_fatal("load_uv_coords: Cannot open '%s' because : %s", file_name, strerror(errno));
	}
	for (p=0; p<nb_pts; p++) {
		if (2!=fscanf(file, "%f %f", &uv->c[p*2], &uv->c[p*2+1])) {
			gltv_log_fatal("load_uv_coords: bad syntax");
		}
	}
	fclose(file);
	len = strlen(file_name)+1;
	uv->name = gltv_memspool_alloc(len);
	if (!uv->name) {
		gltv_log_fatal("load_coords_uv: Cannot get memory for name");
	}
	memcpy(uv->name, file_name, len);
	gltv_hash_put(coords, (unsigned)(uv->name), uv);
	return uv;
}

static GLTV_HASH meshes = NULL;
static gltv_stack *dup_meshes = NULL;

static void data_free_meshes()
{
	unsigned dummy_key;
	mesh *m;
	gltv_hash_reset(meshes);
	while (gltv_hash_each(meshes, &dummy_key, (void**)&m)) {
		gltv_memspool_unregister(m->name);
		gltv_memspool_unregister(m);
	}
	gltv_hash_del(meshes);
}

void data_free_dup_meshes()
{
	mesh *m;
	unsigned i;
	for (i=0; i<gltv_stack_size(dup_meshes); i++) {
		m = gltv_stack_pop(dup_meshes);
		gltv_memspool_unregister(m);
	}
	gltv_stack_del(dup_meshes);
}

mesh *data_load_mesh(const char *file_name, int recentre)
{	/* also loads all necessary datas - reentrant */
	FILE *file;
	char line[LINE_LENGTH];
	mesh *m, *m2;
	size_t len;
	if (NULL==meshes) {
		meshes = gltv_hash_new(30, 2, GLTV_HASH_STRKEYS);
		if (!meshes) {
			gltv_log_fatal("Cannot init hash for meshes");
		}
		atexit(data_free_meshes);
	}
	if (NULL==dup_meshes) {
		dup_meshes = gltv_stack_new(5, 1);
		if (!dup_meshes) {
			gltv_log_fatal("Cannot init stack for duplicate meshes");
		}
		atexit(data_free_dup_meshes);
	}
	m = gltv_memspool_alloc(sizeof(*m));
	if (!m) {
		gltv_log_fatal("Cannot get memory for mesh");
	}
	if (gltv_hash_get(meshes, (unsigned)file_name, (void**)&m2)) {
		/* Its allowed to instanciate a mesh severall times.
		 * Then, coord_uv are merged, and we memorize only the first that was instancied.
		 * Later, this mesh can be bound to another position */
		/* TODO : keep the last one and chain them together so that we can free em all */
		gltv_log_warning(GLTV_LOG_OPTIONAL, "forking mesh '%s'", m2->name);
		memcpy(m,m2,sizeof(*m));
		m->pos = NULL;
		if (!gltv_stack_push(dup_meshes, m)) {
			gltv_log_fatal("Cannot push a duplicate mesh");
		}
		return m;
	}
	gltv_log_warning(GLTV_LOG_DEBUG, "Reading mesh from file '%s'\n", file_name);
	if (NULL==(file=fopen(file_name,"r"))) {
		gltv_log_fatal("load_mesh: Cannot open '%s' because : %s", file_name, strerror(errno));
	}
	m->texture = NULL;
	m->prim = NULL;
	m->uv_coord = NULL;
	m->name = NULL;
	m->pos = NULL;
	while (NULL!=fgets(line, LINE_LENGTH, file)) {
		len = strlen(line);
		if (len<2) continue;
		line[--len] = '\0';
		if (0==strncasecmp(line, "texture", 7)) {
			if (NULL==fgets(line, LINE_LENGTH, file)) {
bad_mesh:
				gltv_log_fatal("Bad mesh");
			}
			len = strlen(line);
			if (len<2) goto bad_mesh;
			line[--len] = '\0';
			m->texture = data_load_texture(line);
			if (NULL==m->texture) goto bad_mesh;
		} else if (0==strncasecmp(line, "primitive", 9)) {
			if (NULL==fgets(line, LINE_LENGTH, file)) goto bad_mesh;
			len = strlen(line);
			if (len<2) goto bad_mesh;
			line[--len] = '\0';
			m->prim = data_load_primitive(line, recentre);
			if (NULL==m->prim) goto bad_mesh;
		} else if (0==strncasecmp(line, "coord_uv", 8)) {
			if (NULL==fgets(line, LINE_LENGTH, file)) goto bad_mesh;
			len = strlen(line);
			if (len<2) goto bad_mesh;
			line[--len] = '\0';
			if (NULL==m->prim) goto bad_mesh;
			m->uv_coord = data_load_uv_coords(line, m->prim->nb_points);
			if (NULL==m->uv_coord) goto bad_mesh;
		} else if (0==strncasecmp(line, "position", 8)) {
			position *pos;
			if (NULL==fgets(line, LINE_LENGTH, file)) goto bad_mesh;
			len = strlen(line);
			if (len<2) goto bad_mesh;
			line[--len] = '\0';
			pos = data_load_position(line);
			if (NULL==pos) goto bad_mesh;
			/* bound the mesh to this position. Later, the mesh can be bind elsewhere */
			data_bind_mesh(pos, m);
		} else {
			gltv_log_warning(GLTV_LOG_MUSTSEE, "Bad syntax line '%s'", line);
		}
	}
	fclose(file);
	len = strlen(file_name)+1;
	m->name = gltv_memspool_alloc(len);
	if (!m->name) {
		gltv_log_warning(GLTV_LOG_MUSTSEE, "load_mesh: Cannot get memory for name");
		gltv_memspool_unregister(m);
		return NULL;
	}
	memcpy(m->name, file_name, len);
	gltv_hash_put(meshes, (unsigned)(m->name), m);
	return m;
}

enigm *data_load_enigm(const char *file_name)
{
	FILE *file;
	char line[LINE_LENGTH];
	enigm *e;
	unsigned nb_bricks_max;
	void *temp;
	size_t len;
	gltv_log_warning(GLTV_LOG_DEBUG, "Reading enigm from file '%s'\n",file_name);
	if (NULL==(file=fopen(file_name,"r"))) {
		gltv_log_warning(GLTV_LOG_MUSTSEE, "data_load_enigm: Cannot open '%s' because : %s", file_name, strerror(errno));
		return NULL;
	}
	e = gltv_memspool_alloc(sizeof(*e));
	if (!e) {
enigm_nomem:
		gltv_log_fatal("load_enigm: Cannot get memory");
	}
	e->nb_bricks = 0;
	nb_bricks_max = 100;
	e->bricks = gltv_memspool_alloc(sizeof(*e->bricks)*nb_bricks_max);
	e->solution_pos = gltv_memspool_alloc(sizeof(*e->solution_pos)*nb_bricks_max);
	if (!e->bricks) goto enigm_nomem;
	while (NULL!=fgets(line, LINE_LENGTH, file)) {
		len = strlen(line);
		if (len<2) continue;
		line[--len] = '\0';
		if (0==strncasecmp(line, "score", 5)) {
			if (NULL==fgets(line, LINE_LENGTH, file)) {
bad_enigm:
				gltv_log_fatal("Bad format");
			}
			e->score = atoi(line);
		} else if (0==strncasecmp(line, "solution", 8)) {
			if (NULL==fgets(line, LINE_LENGTH, file)) goto bad_enigm;
			len = strlen(line);
			if (len<2) goto bad_enigm;
			line[--len] = '\0';
			if (!(e->solution=gltv_memspool_alloc(len+1))) goto enigm_nomem;
			strncpy(e->solution, line, len+1);
		} else if (0==strncasecmp(line, "name", 4)) {
			if (NULL==fgets(line, LINE_LENGTH, file)) goto bad_enigm;
			len = strlen(line);
			if (len<2) goto bad_enigm;
			line[--len] = '\0';
			if (!(e->name=gltv_memspool_alloc(len+1))) {
				goto enigm_nomem;
			}
			strncpy(e->name, line, len+1);
		} else {
			mesh *m;
			if (!gltv_hash_get(enigm_primitives, (unsigned)line, (void**)&m)) {	/* TODO : charger les primitives au fur et à mesure des besoins */
				gltv_log_fatal("Unknow primitive : '%s'\n",line);
			}
			if (e->nb_bricks == nb_bricks_max) {
				nb_bricks_max <<= 1;
				temp = gltv_memspool_realloc(e->bricks, sizeof(*e->bricks)*(nb_bricks_max));
				if (!temp) goto enigm_nomem;
				e->bricks = temp;
				temp = gltv_memspool_realloc(e->solution_pos, sizeof(*e->solution_pos)*(nb_bricks_max));
				if (!temp) goto enigm_nomem;
				e->solution_pos = temp;
			}
			e->bricks[e->nb_bricks] = m;
			if (16!=fscanf(file, "%f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f",
						&e->solution_pos[e->nb_bricks][0], &e->solution_pos[e->nb_bricks][1], &e->solution_pos[e->nb_bricks][2], &e->solution_pos[e->nb_bricks][3],
						&e->solution_pos[e->nb_bricks][4], &e->solution_pos[e->nb_bricks][5], &e->solution_pos[e->nb_bricks][6], &e->solution_pos[e->nb_bricks][7],
						&e->solution_pos[e->nb_bricks][8], &e->solution_pos[e->nb_bricks][9], &e->solution_pos[e->nb_bricks][10], &e->solution_pos[e->nb_bricks][11],
						&e->solution_pos[e->nb_bricks][12], &e->solution_pos[e->nb_bricks][13], &e->solution_pos[e->nb_bricks][14], &e->solution_pos[e->nb_bricks][15])) goto bad_enigm;
			memcpy(e->bricks[e->nb_bricks]->pos->c, &e->solution_pos[e->nb_bricks][0], sizeof(e->bricks[e->nb_bricks]->pos->c));
			e->nb_bricks++;
		}
	}
	temp = gltv_memspool_realloc(e->bricks, sizeof(*e->bricks)*e->nb_bricks);
	if (!temp) {
		gltv_log_warning(GLTV_LOG_MUSTSEE, "Warning : cannot resize down mem\n");
	} else {
		e->bricks = temp;
	}
	temp = gltv_memspool_realloc(e->solution_pos, sizeof(*e->solution_pos)*e->nb_bricks);
	if (!temp) {
		gltv_log_warning(GLTV_LOG_MUSTSEE, "Warning : cannot resize down mem\n");
	} else {
		e->solution_pos = temp;
	}
	e->root = csg_build_tree(e->solution, e->nb_bricks, e->bricks);
	if (NULL==e->root) {
		gltv_log_fatal("data_load_enigm: Cannot build csg tree for '%s'\n", e->name);
	}
	fclose(file);
	return e;
}

void data_enigm_del(enigm *e)
{
	if (e->bricks) gltv_memspool_unregister(e->bricks);
	if (e->solution) gltv_memspool_unregister(e->solution);
	if (e->solution_pos) gltv_memspool_unregister(e->solution_pos);
	if (e->name) gltv_memspool_unregister(e->name);
	if (e->root) csg_node_del(e->root);
	gltv_memspool_unregister(e);
}

enigm *data_enigm_full_new(char *name)
{
	enigm *e;
	unsigned u;
	e = gltv_memspool_alloc(sizeof(*e));
	if (!e) {
full_mem_error:
		gltv_log_fatal("data_enigm_full_new : Cannot get mem");
	}
	e->nb_bricks = gltv_hash_size(enigm_primitives);
	e->bricks = gltv_memspool_alloc(sizeof(*e->bricks)*e->nb_bricks);
	gltv_hash_reset(enigm_primitives);
	for (u=0; u<e->nb_bricks; u++) {
		unsigned dummy;
		char ret;
		ret = gltv_hash_each(enigm_primitives, &dummy, (void**)&e->bricks[u]);
		assert(ret);
	}
	e->solution_pos = gltv_memspool_alloc(sizeof(*e->solution_pos)*e->nb_bricks);
	if (!e->bricks || !e->solution_pos) goto full_mem_error;
	for (u=0; u<e->nb_bricks; u++) {
		geom_set_identity(e->solution_pos[u]);
	}
	e->solution = NULL;
	e->score = 0;
	e->name = name;
	return e;
}

void data_enigm_finalize(enigm *e, char *solution)
{
	size_t len = strlen(solution);
	static char prim_is_used[NB_MAX_PRIMS_IN_TREE];
	static char prim_name[NB_MAX_PRIMS_IN_TREE];
	unsigned p, c, b;
	e->solution = gltv_memspool_alloc(len+1);
	if (!e->solution) gltv_log_fatal("data_enigm_finalize : Cannot get mem");
	memcpy(e->solution, solution, len+1);
	for (p=0; p<e->nb_bricks; p++) {
		prim_is_used[p] = 0;
		prim_name[p] = 'a'+p;
	}
	for (c=0; c<len; c++)
		if (e->solution[c]>='a' && e->solution[c]<='z')
			prim_is_used[e->solution[c]-'a']=1;
	for (c='a', p=0; p<e->nb_bricks; p++) {
		if (prim_is_used[p]) {
			prim_name[p] = c;
			c++;
		}
	}
	for (c=0; c<len; c++) {
		if (e->solution[c]>='a' && e->solution[c]<='z') {
			e->solution[c] = prim_name[e->solution[c]-'a'];
		}
	}
	c = e->nb_bricks;
	for (b=0, p=0; p<c; p++) {
		if (!prim_is_used[p]) {
			memmove(&e->bricks[b], &e->bricks[b+1], sizeof(*e->bricks)*(e->nb_bricks-b-1));
			memmove(e->solution_pos[b], e->solution_pos[b+1], sizeof(*e->solution_pos)*(e->nb_bricks-b-1));
			e->nb_bricks--;
		} else {
			b++;
		}
	}
	e->root = csg_build_tree(e->solution, e->nb_bricks, e->bricks);
	if (NULL==e->root) {
		gltv_log_fatal("data_enigm_finalize: Cannot build csg tree for '%s'\n", e->name);
	}
	/* TODO resize down mem for bricks and solution_pos */
}

void data_enigm_save(enigm *e)
{
	char file_name[256];
	size_t len = strlen(e->name);
	FILE *file;
	unsigned p;
	memcpy(file_name, e->name, len);
	memcpy(file_name+len, ".enigm", 7);
	file = fopen(file_name, "w+");
	if (file==NULL) gltv_log_warning(GLTV_LOG_MUSTSEE, "data_enigm_save : cannot create file '%s'", file_name);
	fprintf(file, "score\n0\nname\n%s\n", e->name);
	for (p=0; p<e->nb_bricks; p++) {
		fprintf(file, "%s\n", e->bricks[p]->name);
		fprintf(file, "%f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f\n",
				e->solution_pos[p][0], e->solution_pos[p][1], e->solution_pos[p][2], e->solution_pos[p][3],
				e->solution_pos[p][4], e->solution_pos[p][5], e->solution_pos[p][6], e->solution_pos[p][7],
				e->solution_pos[p][8], e->solution_pos[p][9], e->solution_pos[p][10], e->solution_pos[p][11],
				e->solution_pos[p][12], e->solution_pos[p][13], e->solution_pos[p][14], e->solution_pos[p][15]);
	}
	fprintf(file, "solution\n%s\n", e->solution);
	fclose(file);
}

void data_enigm_add(enigm *e)
{
	gltv_slist_insert(editable_enigms, e);
}

void data_save_all()
{
	/* we save in userland directory the enigms created by the user, ie enigms curently in edition mode and free enigms */ 
	FILE *file = 0;
	unsigned nb_enigms, i;
	if (NULL==(file=fopen("editable_enigms.lst","w+"))) gltv_log_warning(GLTV_LOG_MUSTSEE, "data_save_all: Cannot open editable_enigms.lst : %s", strerror(errno));
	nb_enigms = gltv_slist_size(editable_enigms);
	for (i=0; i<nb_enigms; i++) {
		enigm *e = gltv_slist_get(editable_enigms, i);
		if (file) fprintf(file, "%s.enigm\n", e->name);
		data_enigm_del(e);
	}
	if (file) fclose(file);
	gltv_slist_del(editable_enigms);
	file = 0;
	if (NULL==(file=fopen("enigms.lst","w+"))) gltv_log_warning(GLTV_LOG_MUSTSEE, "data_save_all: Cannot open enigms.lst : %s", strerror(errno));
	nb_enigms = gltv_slist_size(enigms);
	for (i=0; i<nb_enigms; i++) {
		enigm *e = gltv_slist_get(enigms, i);
		if (file && 0==e->score) fprintf(file, "%s.enigm\n", e->name);
		data_enigm_del(e);
	}
	if (file) fclose(file);
	gltv_slist_del(enigms);
	gltv_hash_del(enigm_primitives);	/* primitives themselves will be destroyed with the destruction of the hash primitives */
}

void data_read_enigms(const char *file_name, GLTV_SLIST enigm_list)
{
	FILE *file;
	char line[LINE_LENGTH];
	if (NULL==(file=fopen(file_name,"r"))) {
		gltv_log_warning(GLTV_LOG_DEBUG, "data_read_enigms: Cannot open '%s' because : %s", file_name, strerror(errno));
		return;
	}
	while (NULL!=fgets(line, LINE_LENGTH, file)) {
		size_t len = strlen(line);
		enigm *e;
		if (len==0 || '\n'!=line[len-1]) {
enigm_error:
			gltv_log_fatal("data_load_enigms: Bad syntax");
		}
		if (len==1) continue;
		line[--len] = '\0';
		e = data_load_enigm(line);
		if (NULL==e) goto enigm_error;
		gltv_log_warning(GLTV_LOG_DEBUG, "data_read_enigms: Load enigm '%s' from '%s'\n", e->name, file_name);
		gltv_slist_insert(enigm_list, e);
	}
	fclose(file);
}

void data_read_all()
{
	/* start by all the geometry */
	FILE *file1;
	char line[LINE_LENGTH];
	enigm_primitives = gltv_hash_new(30, 3, GLTV_HASH_OPT_SIZE|GLTV_HASH_STRKEYS);
	if (!enigm_primitives) gltv_log_fatal("data_read_all: Cannot get hash");
	if (NULL==(file1=fopen("primitives.lst","r"))) gltv_log_fatal("data_read_all: Cannot open primitives.lst : %s", strerror(errno));
	while (NULL!=fgets(line, LINE_LENGTH, file1)) {
		size_t len = strlen(line);
		mesh *m;
		if (len==0 || '\n'!=line[len-1]) {
prim_error:
			gltv_log_fatal("data_load_all: Bad primitive");
		}
		if (len==1) continue;
		line[--len] = '\0';
		m = data_load_mesh(line, 1);
		if (NULL==m) goto prim_error;
		gltv_hash_put(enigm_primitives, (unsigned)(m->name), m);
		gltv_log_warning(GLTV_LOG_DEBUG, "Adding primitive '%s'\n", m->name);
	}
	fclose(file1);
	/* then all enigms */
	enigms = gltv_slist_new(40, 1, enigm_cmp);
	editable_enigms = gltv_slist_new(30, 1, enigm_cmp);
	if (!enigms || !editable_enigms) gltv_log_fatal("data_read_all: Cannot get slist");
	atexit(data_save_all);
	data_read_enigms("enigms.lst", enigms);
	data_read_enigms("editable_enigms.lst", editable_enigms);
}

void data_read_userland()
{
	data_read_enigms("enigms.lst", enigms);
	data_read_enigms("editable_enigms.lst", editable_enigms);
}

