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
#include "png2gl.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <png.h>
#include "memspool.h"
#include "log.h"
#include "opengl.h"


void png2gl_init()
{
}

GLuint png2gl_load_image(char *file_name)
{
	FILE *fp = fopen(file_name, "rb");
	unsigned char header[8];
	static GLuint tex_name = 1;
	png_structp png_ptr;
	png_infop info_ptr;
	png_infop end_info;
	png_bytep *row_pointers;
	png_uint_32 width, height;
	int bit_depth, color_type;
	unsigned line_size;
	unsigned char *image;
	unsigned y;
	if (!fp) {
		gltv_log_fatal("png2gl_load_image '%s' : %s", file_name, strerror(errno));
	}
	fread(header, 1, sizeof(header), fp);
	if (png_sig_cmp(header, 0, sizeof(header))) {
		gltv_log_fatal("png2gl_load_image '%s' : Not a png file", file_name);
	}
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr) {
libpng_error:
		gltv_log_fatal("png2gl_load_image '%s' : libpng error", file_name);
	}
	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
		goto libpng_error;
	}
	end_info = png_create_info_struct(png_ptr);
	if (!end_info) {
		png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
		goto libpng_error;
	}
	
	if (setjmp(png_jmpbuf(png_ptr))) {
		png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
		fclose(fp);
		goto libpng_error;
	}
	png_init_io(png_ptr, fp);
	png_set_sig_bytes(png_ptr, sizeof(header));
	png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_STRIP_16|PNG_TRANSFORM_PACKING|PNG_TRANSFORM_PACKSWAP, NULL);
	row_pointers = png_get_rows(png_ptr, info_ptr);

	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, NULL,NULL,NULL);
	if (color_type!=PNG_COLOR_TYPE_RGB && color_type!=PNG_COLOR_TYPE_RGB_ALPHA) {
		png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
		fclose(fp);
		gltv_log_fatal("png2gl_load_image '%s' : not a RBG(A) image", file_name);
	}
	line_size = width*(color_type==PNG_COLOR_TYPE_RGB?3:4);
	image = gltv_memspool_alloc(height*line_size);
	if (!image) {
		png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
		fclose(fp);
		gltv_log_fatal("png2gl_load_image '%s' : cant get memory", file_name);
	}
	for (y=0; y<height; y++) {
		memcpy(image+(height-y-1)*line_size, row_pointers[y], line_size);
	}
	
	glBindTexture(GL_TEXTURE_2D, tex_name);
	glTexImage2D(GL_TEXTURE_2D, 0, color_type==PNG_COLOR_TYPE_RGB ? GL_RGB : GL_RGBA, width, height, 0, color_type==PNG_COLOR_TYPE_RGB ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, image);
	gltv_memspool_unregister(image);

	png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
	fclose(fp);
	return tex_name++;
}
