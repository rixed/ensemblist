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
#include <assert.h>
#include "opengl.h"
#include "draw.h"
#include "csg.h"
#include "data.h"
#include "main.h"
#include "memspool.h"
#include "log.h"
#include "geom.h"

static int stencil_size;	/* in bits */

PFNGLNEWBUFFERREGIONPROC glNewBufferRegion = NULL;
PFNGLDELETEBUFFERREGIONPROC glDeleteBufferRegion = NULL;
PFNGLREADBUFFERREGIONPROC glReadBufferRegion = NULL;
PFNGLDRAWBUFFERREGIONPROC glDrawBufferRegion = NULL;
PFNGLBUFFERREGIONENABLEDPROC glBufferRegionEnabled = NULL;

#ifdef _WIN32
PFNWGLCREATEBUFFERREGIONARBPROC wglCreateBufferRegionARB = NULL;
PFNWGLDELETEBUFFERREGIONARBPROC wglDeleteBufferRegionARB = NULL;
PFNWGLSAVEBUFFERREGIONARBPROC wglSaveBufferRegionARB = NULL;
PFNWGLRESTOREBUFFERREGIONARBPROC wglRestoreBufferRegionARB = NULL;
static HANDLE wgl_region;

void wgl_free()
{
	wglDeleteBufferRegionARB(wgl_region);
}
#endif

static GLuint ktx_region;
static char ktx_ok, wgl_ok;

#ifdef __APPLE__
#import <mach-o/dyld.h>
#import <string.h>
static void *NSGLGetProcAddress(const char *name)
{
	NSSymbol symbol;
	char *symbolName;
	// Prepend a '_' for the Unix C symbol mangling convention
	symbolName = malloc (strlen (name) + 2);
	strcpy(symbolName + 1, name);
	symbolName[0] = '_';
	symbol = NULL;
	if (NSIsSymbolNameDefined (symbolName))
		symbol = NSLookupAndBindSymbol (symbolName);
	free (symbolName);
	return symbol ? NSAddressOfSymbol (symbol) : NULL;
}
#endif

static void *lglGetProcAddress(const char *name)
{
#ifdef _WIN32
	void *t = (void*)wglGetProcAddress(name);
	if (t == 0)
		gltv_log_warning(GLTV_LOG_MUSTSEE, "wglGetProcAddress know nothing about '%s'", name);
	return NULL;
#elif __APPLE__ 
	void *t = (void*)NSGLGetProcAddress(name);
	if (t == 0)
		gltv_log_warning(GLTV_LOG_MUSTSEE, "NSGLGetProcAddress know nothing about '%s'", name);
	return t;
#else
	void *t = (void*)glXGetProcAddressARB(name);
	if (t == 0)
		gltv_log_warning(GLTV_LOG_MUSTSEE, "glXGetProcAddress know nothing about '%s'", name);
	return t;
#endif
}

void ktx_free()
{
	glDeleteBufferRegion(ktx_region);
}

Display *display;
GLXPbuffer pbuffer;
GLXDrawable current_read, current_draw;
GLXContext current_context;
void pbuffer_free()
{
	glXDestroyPbuffer(display, pbuffer);
}

int with_ktx = 0, with_wgl = 0;
void draw_init()
{
	GLTV_WARN_LEVEL level = GLTV_LOG_OPTIONAL;
	/* general info about opengl */
	gltv_log_warning(GLTV_LOG_OPTIONAL, "OpenGl Vendor : %s", glGetString(GL_VENDOR));
	gltv_log_warning(GLTV_LOG_OPTIONAL, "OpenGl Renderer : %s", glGetString(GL_RENDERER));
	gltv_log_warning(GLTV_LOG_OPTIONAL, "OpenGl Version : %s", glGetString(GL_VERSION));
	gltv_log_warning(GLTV_LOG_OPTIONAL, "OpenGl Extensions : %s", glGetString(GL_EXTENSIONS));
	/* draw specific reset */
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glGetIntegerv(GL_STENCIL_BITS, &stencil_size);
	glClearDepth((GLfloat)1);
	if (stencil_size<4) level = GLTV_LOG_MUSTSEE;
	gltv_log_warning(level, "Stencil size : %d bits\n", stencil_size);
	ktx_ok = 0;
	if (with_ktx) {
		/* get KTX functions */
		glNewBufferRegion = (PFNGLNEWBUFFERREGIONPROC)lglGetProcAddress("glNewBufferRegion");
		glDeleteBufferRegion = (PFNGLDELETEBUFFERREGIONPROC)lglGetProcAddress("glDeleteBufferRegion");
		glReadBufferRegion = (PFNGLREADBUFFERREGIONPROC)lglGetProcAddress("glReadBufferRegion");
		glDrawBufferRegion = (PFNGLDRAWBUFFERREGIONPROC)lglGetProcAddress("glDrawBufferRegion");
		glBufferRegionEnabled = (PFNGLBUFFERREGIONENABLEDPROC)lglGetProcAddress("glBufferRegionEnabled");
		if (glNewBufferRegion && glDeleteBufferRegion && glReadBufferRegion && glDrawBufferRegion && glBufferRegionEnabled) {
			if (glBufferRegionEnabled()) {
				gltv_log_warning(GLTV_LOG_MUSTSEE, "KTX extention in use :-)");
				ktx_region = glNewBufferRegion(GL_KTX_Z_REGION);
				atexit(ktx_free);
				ktx_ok = 1;
			} else {
				gltv_log_warning(GLTV_LOG_MUSTSEE, "KTX extention not usable :'(");
				ktx_ok = 0;
			}
		} else {
			gltv_log_warning(GLTV_LOG_MUSTSEE, "KTX extention not supported");
			ktx_ok = 0;
		}
#ifdef _WIN32
	} else if (with_wgl) {
		wglCreateBufferRegionARB = (PFNWGLCREATEBUFFERREGIONARBPROC) lglGetProcAddress("wglCreateBufferRegionARB");
		wglDeleteBufferRegionARB = (PFNWGLDELETEBUFFERREGIONARBPROC) lglGetProcAddress("wglDeleteBufferRegionARB");
		wglSaveBufferRegionARB = (PFNWGLSAVEBUFFERREGIONARBPROC) lglGetProcAddress("wglSaveBufferRegionARB");
		wglRestoreBufferRegionARB = (PFNWGLRESTOREBUFFERREGIONARBPROC) lglGetProcAddress("wglRestoreBufferRegionARB");
		if (wglCreateBufferRegionARB && wglDeleteBufferRegionARB && wglSaveBufferRegionARB && wglRestoreBufferRegionARB) {
			gltv_log_warning(GLTV_LOG_MUSTSEE, "WGL extention in use :-)");
			wgl_region = wglCreateBufferRegionARB(wglGetCurrentDC(), 0, WGL_DEPTH_BUFFER_BIT_ARB);
			atexit(wgl_free);
			wgl_ok = 1;
		} else {
			gltv_log_warning(GLTV_LOG_MUSTSEE, "WGL extention not in use :'(");
		}
#endif
	}
}

void draw_primitive(primitive *prim)
{
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, prim->points);
	if (NULL!=prim->norms) {
		glEnableClientState(GL_NORMAL_ARRAY);
		glNormalPointer(GL_FLOAT, 0, prim->norms);
	}
	glDrawElements(GL_TRIANGLES, (int)prim->nb_faces*3, GL_UNSIGNED_INT, (void*)prim->faces);
}

void draw_mesh(mesh *m)
{
	if (NULL!=m->texture && NULL!=m->uv_coord) {
		glBindTexture(GL_TEXTURE_2D, m->texture->binding);
		glEnable(GL_TEXTURE_2D);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		glTexCoordPointer(2, GL_FLOAT, 0, (void*)m->uv_coord->c);
	} else {
		glDisable(GL_TEXTURE_2D);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}
	draw_primitive(m->prim);
}

static void draw_node(csg_node *node)
{
	/* draw the primitive, at the location */
	assert(node->type == CSG_PRIM);
	glPushMatrix();
	glMultMatrixf(node->u.prim.m->pos->c);
	draw_mesh(node->u.prim.m);
	glPopMatrix();
}

static GLfloat *depth_buffer = NULL;
static unsigned buffer_size = 0;
static char buffer_region_succeeded;
GLfloat *depthSave = NULL;
GLubyte *stencilSave = NULL;
GLubyte *colorSave = NULL;

static void free_buffers()
{
	if (depth_buffer) gltv_memspool_unregister(depth_buffer);
	if (depthSave) gltv_memspool_unregister(depthSave);
	if (stencilSave) gltv_memspool_unregister(stencilSave);
	if (colorSave) gltv_memspool_unregister(colorSave);
}

void push_ortho_view(float left, float right, float bottom, float top, float znear, float zfar)
{
	glPushMatrix();
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(left, right, bottom, top, znear, zfar);
}

void pop_view(void)
{
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}

static void zbuffer_save(b_box_2d *region)
{
	unsigned needed_size;
	if (region->vide) return;
	if (ktx_ok) {
		glReadBufferRegion(ktx_region, region->xmin, region->ymin, region->xmax-region->xmin, region->ymax-region->ymin);
		return;
#ifdef _WIN32
	} else if (wgl_ok) {
		if (GL_TRUE==wglSaveBufferRegionARB(wgl_region, region->xmin, region->ymin, region->xmax-region->xmin, region->ymax-region->ymin)) {
			buffer_region_succeeded = 1;
			return;
		} else {
			buffer_region_succeeded = 0;
		}
#endif
	}
	needed_size = (region->xmax-region->xmin)*(region->ymax-region->ymin)*sizeof(GLuint);
	if (NULL == depth_buffer) {
		depth_buffer = gltv_memspool_alloc(needed_size);
		if (!depth_buffer) gltv_log_fatal("Cannot get memory for saving depth buffer");
		buffer_size = needed_size;
		atexit(free_buffers);
	} else if (buffer_size<needed_size) {
		depth_buffer = gltv_memspool_realloc(depth_buffer, needed_size);
		if (!depth_buffer) gltv_log_fatal("Cannot get memory for saving depth buffer");
		buffer_size = needed_size;
	}
	glReadPixels(region->xmin, region->ymin, region->xmax-region->xmin, region->ymax-region->ymin, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, depth_buffer);
}

void zbuffer_restore(b_box_2d *region)
{
	if (region->vide) return;
	if (ktx_ok) {
		glDrawBufferRegion(ktx_region, region->xmin, region->ymin, region->xmax-region->xmin, region->ymax-region->ymin, region->xmin, region->ymin);
		return;
#ifdef _WIN32
	} else if (wgl_ok && buffer_region_succeeded) {
		wglRestoreBufferRegionARB(wgl_region, region->xmin, region->ymin, region->xmax-region->xmin, region->ymax-region->ymin, region->xmin, region->ymin);
		return;
#endif
	}
	glDisable(GL_STENCIL_TEST);
	glColorMask(0, 0, 0, 0);
	glDepthMask(1);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_ALWAYS);
	push_ortho_view(0., (float)glut_fenLong, 0., (float)glut_fenHaut, 0., 1.);
	glRasterPos3f(region->xmin, region->ymin, -.5);
	glDrawPixels(region->xmax-region->xmin, region->ymax-region->ymin, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, depth_buffer);
/*	glColorMask(1,1,1,1);
	glBegin(GL_LINE_STRIP);
	glColor3f(1,1,1);
	glVertex3f(region->xmin, region->ymin, -.5);
	glVertex3f(region->xmax, region->ymin, -.5);
	glVertex3f(region->xmax, region->ymax, -.5);
	glVertex3f(region->xmin, region->ymax, -.5);
	glVertex3f(region->xmin, region->ymin, -.5);
	glEnd();
	glColorMask(0,0,0,0);*/
	pop_view();
	glDepthFunc(GL_LESS);
	glEnable(GL_STENCIL_TEST);
}

void zbuffer_clear(b_box_2d *region)
{
	if (region->vide) return;
	push_ortho_view(0., (float)glut_fenLong, 0., (float)glut_fenHaut, 0., 1.);
	glBegin(GL_QUADS);
	glVertex3f(region->xmin, region->ymax, -1);
	glVertex3f(region->xmax, region->ymax, -1);
	glVertex3f(region->xmax, region->ymin, -1);
	glVertex3f(region->xmin, region->ymin, -1);
	glEnd();
	pop_view();
}

#ifndef NDEBUG
void copyDepthToColor()
{
	int x, y;
	GLfloat max, min;
	GLint previousColorBuffer;
	unsigned needed_size = glut_fenLong*glut_fenHaut*sizeof(GLfloat);
	static unsigned size = 0;
	int winWidth = glut_fenLong, winHeight=glut_fenHaut;
	if (depthSave==NULL) {
		depthSave = gltv_memspool_alloc(needed_size);
		size = needed_size;
	} else if (needed_size>size) {
		depthSave = gltv_memspool_realloc(depthSave, needed_size);
		size = needed_size;
	}
	glReadPixels(0, 0, winWidth, winHeight, GL_DEPTH_COMPONENT, GL_FLOAT, depthSave);
	/* I'm sure this could be done much better with OpenGL */
	max = 0;
	min = 1;
	for(y = 0; y < winHeight; y++)
		for(x = 0; x < winWidth; x++) {
			if(depthSave[winWidth * y + x] < min)
				min = depthSave[winWidth * y + x];
			if(depthSave[winWidth * y + x] > max)// && depthSave[winWidth * y + x] < .999)
				max = depthSave[winWidth * y + x];
		}
	for(y = 0; y < winHeight; y++)
		for(x = 0; x < winWidth; x++) {
			if(depthSave[winWidth * y + x] <= max)
				depthSave[winWidth * y + x] = 1 -  (depthSave[winWidth * y + x] - min) / (max - min);
			else
				depthSave[winWidth * y + x] = 0;
		}
	push_ortho_view(0., 1., 0., 1., 0., 1.);
	glRasterPos3f(0, 0, -.5);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_STENCIL_TEST);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glGetIntegerv(GL_DRAW_BUFFER, &previousColorBuffer);
	glDrawBuffer(GL_BACK);
	glDrawPixels(winWidth, winHeight, GL_LUMINANCE , GL_FLOAT, depthSave);
	glDrawBuffer(previousColorBuffer);
	glEnable(GL_STENCIL_TEST);
	glEnable(GL_DEPTH_TEST);
	pop_view();
}

unsigned char stencilValueToColorMap[][3] = {
	{255, 0, 0},
	{255, 218, 250},
	{72, 255, 0},
	{0, 255, 145},
	{0, 145, 255},
	{255, 255, 255},
	{255, 0, 218}
};

void copyStencilToColor()
{
	int x, y;
	GLint previousColorBuffer;
	int winWidth = glut_fenLong, winHeight=glut_fenHaut;
	unsigned needed_size = glut_fenLong*glut_fenHaut*sizeof(*stencilSave);
	static unsigned size = 0;
	if (stencilSave==NULL) {
		stencilSave = gltv_memspool_alloc(needed_size);
		size = needed_size;
		colorSave = gltv_memspool_alloc(winWidth * winHeight * 4 * sizeof(GLubyte));
	} else if (needed_size>size) {
		stencilSave = gltv_memspool_realloc(stencilSave, needed_size);
		size = needed_size;
		colorSave = gltv_memspool_realloc(colorSave, winWidth * winHeight * 4 * sizeof(GLubyte));
	}
	glReadPixels(0, 0, winWidth, winHeight, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, stencilSave);
	/* I'm sure this could be done much better with OpenGL */
	for(y = 0; y < winHeight; y++)
		for(x = 0; x < winWidth; x++) {
			int stencilValue;
			stencilValue = stencilSave[winWidth * y + x];
/*			if (stencilValue&16) {
				colorSave[(winWidth * y + x) * 3 + 0]=250;
				colorSave[(winWidth * y + x) * 3 + 1]=250;
				colorSave[(winWidth * y + x) * 3 + 2]=250;
			} else {
				colorSave[(winWidth * y + x) * 3 + 0]=0;
				colorSave[(winWidth * y + x) * 3 + 1]=0;
				colorSave[(winWidth * y + x) * 3 + 2]=0;
			}*/
			colorSave[(winWidth * y + x) * 3 + 0] =
				stencilValueToColorMap[stencilValue & 3][0];
			colorSave[(winWidth * y + x) * 3 + 1] =
				stencilValueToColorMap[stencilValue & 3][1];
			colorSave[(winWidth * y + x) * 3 + 2] =
				stencilValueToColorMap[stencilValue & 3][2];
		}
	push_ortho_view(0., 1., 0., 1., 0., 1.);
	glRasterPos3f(0., 0., -.5);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_STENCIL_TEST);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glGetIntegerv(GL_DRAW_BUFFER, &previousColorBuffer);
	glDrawBuffer(GL_BACK);
	glDrawPixels(winWidth, winHeight, GL_RGB, GL_UNSIGNED_BYTE, colorSave);
	glDrawBuffer(previousColorBuffer);
	glEnable(GL_STENCIL_TEST);
	glEnable(GL_DEPTH_TEST);
	pop_view();
}

void draw_b_box(csg_node *node)
{
	b_box b;
	geom_get_b_box(node, &b);
	glBegin(GL_LINE_STRIP);
	glColor3f(.1,1,1);
	glVertex3f(b.p_min[0], b.p_min[1], b.p_min[2]);
	glVertex3f(b.p_max[0], b.p_min[1], b.p_min[2]);
	glVertex3f(b.p_max[0], b.p_max[1], b.p_min[2]);
	glVertex3f(b.p_min[0], b.p_max[1], b.p_min[2]);
	glVertex3f(b.p_min[0], b.p_min[1], b.p_min[2]);	
	glVertex3f(b.p_min[0], b.p_min[1], b.p_max[2]);
	glVertex3f(b.p_max[0], b.p_min[1], b.p_max[2]);
	glVertex3f(b.p_max[0], b.p_max[1], b.p_max[2]);
	glVertex3f(b.p_min[0], b.p_max[1], b.p_max[2]);
	glVertex3f(b.p_min[0], b.p_min[1], b.p_max[2]);
	glEnd();
	glBegin(GL_LINE);
	glVertex3f(b.p_max[0], b.p_min[1], b.p_min[2]);
	glVertex3f(b.p_max[0], b.p_min[1], b.p_max[2]);
	glVertex3f(b.p_max[0], b.p_max[1], b.p_min[2]);
	glVertex3f(b.p_max[0], b.p_max[1], b.p_max[2]);
	glVertex3f(b.p_min[0], b.p_max[1], b.p_min[2]);
	glVertex3f(b.p_min[0], b.p_max[1], b.p_max[2]);
	glEnd();
}
#endif	/* !NDEBUG */

void draw_union_of_partial_products(csg_union_of_partial_products *uopp)
{
	/* On ne peut pas tracer les partiels indendament des groupes, car le zbuffer doit etre integre entre deux vraies unions
	 * (les unions entre les partiels sont des fausses unions justement en ce sens : le zbuffer ne contient pas d'info sur ces unions).
	 * Donc entre deux groupes on DOIT sauver/restaurer le zbuffer.
	 * Mais pas entre partiels d'un meme groupe : la, on utilise le stencil autant que possible pour tout tracer a la fin si possible.
	 * Si pas possible (pas assez de bits dans le stencil), on traite le partiel comme un groupe normal : on restaure le zbuffer, on trace et
	 * on resauvegarde le zbuffer pour continuer le groupe.
	 */
	unsigned g, uu, u = 0, p;
	int a;
	char fake_new_group = 0;
	b_box_2d *region;	/* 2d region of a group (to save only this region) */
	b_box_2d region_full = { 0, 0,0, 0,0 };
	region_full.xmax=glut_fenLong;
	region_full.ymax=glut_fenHaut;
	gltv_log_warning(GLTV_LOG_DEBUG, "------\nDRAW");
	csg_union_of_partial_products_resize(uopp);
	glDisable(GL_TEXTURE_2D);
	while (u<uopp->nb_unions) {
		// new group
		g = uopp->unions[u].group_id;
		// what's its target ?
		if (!fake_new_group) {
			region = &uopp->max2d_group[g];
		}
		gltv_log_warning(GLTV_LOG_DEBUG, "nouveau groupe");
		// else keep the last group_target as this "group" is a fake group, considered as a group because of the lack of stencil bits
		fake_new_group = 0;
		if (1==uopp->unions[u].nb_products) {
			glDepthMask(1);
			glColorMask(1,1,1,1);
			glEnable(GL_LIGHTING);
			glDisable(GL_STENCIL_TEST);
			glEnable(GL_DEPTH_TEST);
			if (uopp->unions[u].products[0].frontal) {
				glCullFace(GL_BACK);
			} else {
				glCullFace(GL_FRONT);
			}
			draw_node(uopp->unions[u].products[0].node);
			gltv_log_warning(GLTV_LOG_DEBUG, "fast draw target %s colors", uopp->unions[u].products[0].node->u.prim.m->name);
			u++;
			continue;
		}
		zbuffer_save(region);
		// classification of the group
		gltv_log_warning(GLTV_LOG_DEBUG, "save zbuffer");
		a = 0;
		glDisable(GL_LIGHTING);
		while (u<uopp->nb_unions && uopp->unions[u].group_id==g) {	// foreach target surface in the group = for each partial in the group
			// draw depth of the target and clear Spacc and Sp, set the Sa-bit
			glDepthMask(1);
			glColorMask(0,0,0,0);
			glDisable(GL_STENCIL_TEST);
			glDepthFunc(GL_ALWAYS);
			glDisable(GL_CULL_FACE);
			zbuffer_clear(region);
			glEnable(GL_CULL_FACE);
			glEnable(GL_STENCIL_TEST);
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_LESS);
			gltv_log_warning(GLTV_LOG_DEBUG, "\tclear zbuffer");
			if (uopp->unions[u].products[0].frontal) {
				glCullFace(GL_BACK);
			} else {
				glCullFace(GL_FRONT);
			}
			glStencilMask(0x3+(1<<(2+a)));
			glStencilFunc(GL_ALWAYS, 1<<(2+a),0);
			glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
			draw_node(uopp->unions[u].products[0].node);
			gltv_log_warning(GLTV_LOG_DEBUG, "\tdraw target %s depth, %s frontal, clearing Sp & Spacc, setting Sa", uopp->unions[u].products[0].node->u.prim.m->name, uopp->unions[u].products[0].frontal?"":"not");	
			// trim the depth values with every trimmers (so that information is stored in Spacc and we use only 2 bits of stencil (Spacc and Sp) -> Spacc=1 when pixels are OUT)
			glDepthMask(0);
			glColorMask(0,0,0,0);
			for (p=1; p<uopp->unions[u].nb_products; p++) {
				glStencilMask(2);	// bit 1 for Sp
				glStencilFunc(GL_ALWAYS, 0,0);
				glStencilOp(GL_KEEP, GL_KEEP, GL_INVERT);
				glDisable(GL_CULL_FACE);
				draw_node(uopp->unions[u].products[p].node);
				gltv_log_warning(GLTV_LOG_DEBUG, "\t\tset Sp to 1 where %s is cut", uopp->unions[u].products[p].node->u.prim.m->name);
				// set Spacc to 1 if Sp=1 or Sp=0 (depending of negation of target)
				glEnable(GL_CULL_FACE);	// restaure it to the target state
				glStencilFunc((!uopp->unions[u].products[p].frontal)?GL_LEQUAL:GL_GREATER, 0x1, 0x3);	// pourquoi ce NOT ??????
				glStencilOp(GL_KEEP, GL_REPLACE, GL_REPLACE);
				glStencilMask(0x3);	// write Sp and Spacc
				draw_node(uopp->unions[u].products[0].node);
				gltv_log_warning(GLTV_LOG_DEBUG, "\t\tset Spacc to trim against %s, %s frontal", uopp->unions[u].products[p].node->u.prim.m->name, uopp->unions[u].products[p].frontal?"":"not");
				if (p<uopp->unions[u].nb_products-1) {
					// draw another time to clear Sp
					glStencilFunc(GL_ALWAYS, 0,0);
					glStencilMask(2);	// clear Sp
					glStencilOp(GL_ZERO, GL_ZERO, GL_ZERO);
					draw_node(uopp->unions[u].products[0].node);
					gltv_log_warning(GLTV_LOG_DEBUG, "\t\tclear Sp");
				}
			}
			// clear bit 2+a of stencil to 0 where Spacc=1 for the target (pixel is out)
			glStencilMask(1<<(2+a));
			glStencilFunc(GL_EQUAL, 1, 1);	// Spacc == 1 ?
			glStencilOp(GL_KEEP, GL_ZERO, GL_ZERO);
			draw_node(uopp->unions[u].products[0].node);
			gltv_log_warning(GLTV_LOG_DEBUG, "\tclear Sa-bit %d for target %s where Spacc=1", (2+a), uopp->unions[u].products[0].node->u.prim.m->name);
			a++;
			u++;
			if (a+2==stencil_size) {
				fake_new_group=1;
				break;
			}
		}
		// restore the zbuffer
		zbuffer_restore(region);
		gltv_log_warning(GLTV_LOG_DEBUG, "restore zbuffer");
		// render the target primitives of that group, trimmed by their stencil a-bits
		glDepthMask(1);
		glColorMask(1,1,1,1);
		glStencilMask(0);
		glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
		glEnable(GL_LIGHTING);
		uu = u-a;
		for (a--; a>=0; a--) {
			if (uopp->unions[uu+a].products[0].frontal) {
				glCullFace(GL_BACK);
			} else {
				glCullFace(GL_FRONT);
			}
			glStencilFunc(GL_EQUAL, 1<<(a+2), 1<<(a+2));
			draw_node(uopp->unions[uu+a].products[0].node);
			gltv_log_warning(GLTV_LOG_DEBUG, "draw target %s colors where Sa-bit is set (%d)", uopp->unions[uu+a].products[0].node->u.prim.m->name, (a+2));
		}
		/*debug
		  glDisable(GL_STENCIL_TEST);
		  draw_b_box(group_target);
		  glEnable(GL_STENCIL_TEST);
		  end_debug*/
	}
	glDisable(GL_STENCIL_TEST);
}

void draw_union_of_products(csg_union_of_products *uop)
{
	/* this convertion should be done earlyer */
	csg_union_of_partial_products *uopp = csg_union_of_partial_products_new(uop);
	if (NULL!=uopp) draw_union_of_partial_products(uopp);
	csg_union_of_partial_products_del(uopp);
}

void draw_csg_tree(csg_node *node)
{
	/* this convertion should be done earlyer */
	csg_union_of_products *uop = csg_union_of_products_new(node);
	if (NULL!=uop) draw_union_of_products(uop);
	csg_union_of_products_del(uop);
}

void draw_position(position *pos)
{
	unsigned s;
	glPushMatrix();
	glMultMatrixf(pos->c);
	for (s=0; s<pos->nb_sons; s++) {
		draw_position(pos->sons[s]);
	}
	for (s=0; s<pos->nb_meshes; s++) {
		draw_mesh(pos->meshes[s]);
	}
	glPopMatrix();
}

