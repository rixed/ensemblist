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
#ifndef OPENGL_H
#define OPENGL_H
#ifdef _WINDOWS
#include <windows.h>
#include <glut.h>
#include <wglext.h>
#include <GL/gl.h>
#else
#ifdef __APPLE_CC__
#include <GLUT/glut.h>
#include <GLUT/glx.h>
#include <OpenGL/gl.h>
#else
#include <GL/glut.h>
#include <GL/glx.h>
#include <GL/gl.h>
#endif
#endif

#ifndef GLAPIENTRY
#ifdef _WIN32
#define GLAPIENTRY __stdcall	/* just in case */
#else
#define GLAPIENTRY
#endif
#endif

/* Borrowed from http://www.west.net/~brittain/3dsmax2.htm#OpenGL
 *
 * This is an explication of how the KTX (Kinetix) extension works.
 *
 ******************************************************************

The OpenGL extension described below, if present, will be used by MAX to implement dual planes
under OpenGL. As with all OpenGL extensions under Windows NT, the functions are imported into
MAX by calling wglGetProcAddress, and the functions themselves are implemented with the __stdcall
calling convention. The presence of this extension is indicated by the keyword "GL_KTX_buffer_region"
being present in the string returned by glGetString(GL_EXTENSIONS).
In an optimal implementation of this extension, the buffer regions are stored in video RAM so
that buffer data transfers do not have to cross the system bus. Note that no data in the backing
buffers is ever interpreted by MAX - it is just returned to the active image and/or Z
buffers later to restore a partially rendered scene without having to actually perform any rendering.
Thus, the buffered data should be kept in the native display card format without any translation.

GLuint glNewBufferRegion(GLenum type)

This function creates a new buffer region and returns a handle to it. The type parameter can be one
of GL_KTX_FRONT_REGION, GL_KTX_BACK_REGION, GL_KTX_Z_REGION or GL_KTX_STENCIL_REGION.
These symbols are defined in the MAX gfx.h header file, but they are simply mapped to 0 through 3 in the
order given above. Note that the storage of this region data is implementation specific and the pixel data
is not available to the client.

void glDeleteBufferRegion(GLuint region)

This function deletes a buffer region and any associated buffer data.

void glReadBufferRegion(GLuint region, GLint x, GLint y, GLsizei width, GLsizei height)

This function reads buffer data into a region specified by the given region handle. The type of
data read depends on the type of the region handle being used. All coordinates are window-based (with the
origin at the lower-left, as is common with OpenGL) and attempts to read areas that are clipped by the
window bounds fail silently. In MAX, x and y are always 0.

void glDrawBufferRegion(GLuint region, GLint x, GLint y, Glsizei width, GLsizei height, GLint xDest, GLint yDest)

This copies a rectangular region of data back to a display buffer. In other words, it moves previously saved
data from the specified region back to its originating buffer. The type of data drawn depends on the type of
the region handle being used. The rectangle specified by x, y, width, and height will always lie completely
within the rectangle specified by previous calls to glReadBufferRegion. This rectangle is to be placed back
into the display buffer at the location specified by xDest and yDest. Attempts to draw sub-regions outside the
area of the last buffer region read will fail (silently). In MAX, xDest and yDest are always equal to x and y,
respectively.)

GLuint glBufferRegionEnabled(void)

This routine returns 1 (TRUE) if MAX should use the buffer region extension, and 0 (FALSE) if MAX shouldn't.
This call is here so that if a single display driver supports a family of display cards with varying functionality
and onboard memory, the extension can be implemented yet only used if a given display card could benefit from
its use. In particular, if a given display card does not have enough memory to efficiently support the buffer
region extension, then this call should return FALSE. (Even for cards with lots of memory, whether or not to
enable the extension could be left up to the end-user through a configuration option available through a
manufacturer's addition to the Windows tabbed Display Properties dialog. Then, those users who like to have as
much video memory available for textures as possible could disable the option, or other users who work with large
scene databases but not lots of textures could explicitly enable the extension.)

Notes: Buffer region data is stored per window. Any context associated with the window can access the
buffer regions for that window. Buffer regions are cleaned up on deletion of the window. MAX uses the buffer
region calls to squirrel away complete copies of each viewport?s image and Z buffers. Then, when a rectangular
region of the screen must be updated because "foreground" objects have moved, that subregion is moved from
"storage" back to the image and Z buffers used for scene display. MAX then renders the objects that have moved
to complete the update of the viewport display.

*/

#ifndef GL_KTX_FRONT_REGION
#define GL_KTX_FRONT_REGION 0
#endif
#ifndef GL_KTX_BACK_REGION
#define GL_KTX_BACK_REGION 1
#endif
#ifndef GL_KTX_Z_REGION
#define GL_KTX_Z_REGION 2
#endif
#ifndef GL_KTX_STENCIL_REGION
#define GL_KTX_STENCIL_REGION 3
#endif

typedef GLuint (GLAPIENTRY * PFNGLNEWBUFFERREGIONPROC) (GLenum type);
typedef void (GLAPIENTRY * PFNGLDELETEBUFFERREGIONPROC) (GLuint region);
typedef void (GLAPIENTRY * PFNGLREADBUFFERREGIONPROC) (GLuint region, GLint x, GLint y, GLsizei width, GLsizei height);
typedef void (GLAPIENTRY * PFNGLDRAWBUFFERREGIONPROC) (GLuint region, GLint x, GLint y, GLsizei width, GLsizei height, GLint xDest, GLint yDest);
typedef GLuint (GLAPIENTRY * PFNGLBUFFERREGIONENABLEDPROC) (void);

#endif
