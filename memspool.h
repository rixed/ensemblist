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
/*
 * Manage a memory area of determined size
 *
 * Some programs can make use of memory if available to raise performences.
 * Then, the user must provide a memory size that the program is allowed to
 * use.
 * Some functions can then tell what's the memory consuption of the program.
 *
 * Of course, there is only one spool available per process.
 *
 * Some functions can register adresses : that is, they wan't the bloc which
 * countain that adress, if any, to stau in memory even when the creator of the bloc will
 * unregister it.
 *
 */

#ifndef _GLTV_MEMSPOOL_H_
#define _GLTV_MEMSPOOL_H_
#include <stdlib.h>
/* Must be called at the begining or after gltv_memspool_end(),
 * with the size in bytes of the spool */
extern int gltv_memspool_init(unsigned new_size);
/* Must be called after all alloced blocks have been freed */
extern void gltv_memspool_end();
/* To get some bytes from the spool.
 * Return 0 if malloc fails.
 * This is not an error when the declared size of the spool is exceeded. */
extern void *gltv_memspool_alloc_d(unsigned requested_size, char const *, size_t);
#define gltv_memspool_alloc(size) gltv_memspool_alloc_d(size, __FILE__, __LINE__)
/* resize the allocated block */
/* like the system call, this function returns the new adress of the block, wich
 * datas are unchanged, or 0 if the realloc fails, in wich case the farmer block
 * still exists */
extern void *gltv_memspool_realloc(void *mem, unsigned new_size);
/* Unregister the memory block so that it can be freed */
extern void gltv_memspool_unregister(void *mem);
/* register the memory block countaining the adress given so that it won't get freed until we unregister from it
 * return 0 if no blocs countained that adress */
extern int gltv_memspool_register(void *mem);
/* To know how many percents of the memory is used */
extern int gltv_memspool_consumption();
/* return the size of a previously allocated block */
extern unsigned gltv_memspool_blocksize(void *mem);
/* stupid thing to check all headers are valid */
extern void gltv_memspool_test();

#endif
