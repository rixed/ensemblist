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
 * Functions to create and manipulate a list of void*.
 */

#ifndef _GLTV_LIST_
#define _GLTV_LIST_

//struct s_gltv_list;
typedef struct s_gltv_list* GLTV_LIST;

/* to get a new empty list */
/* size is the total estimated max size of the list.
 * if resizeable, the list will expand as necessary */
extern GLTV_LIST gltv_list_new(unsigned, int);
/* to delete a list */
extern void gltv_list_del(GLTV_LIST);
/* to push a value - return 1 on succes */
extern int gltv_list_push(GLTV_LIST, void *);
/* to pop a value */
extern void *gltv_list_pop(GLTV_LIST);
/* to empty the list (the list keeps its current size, even if resized) */
extern void gltv_list_clear(GLTV_LIST);
/* tells the list size */
extern unsigned gltv_list_size(GLTV_LIST);
/* returns value n (optimised if last accessed value was not far from n) */
extern void *gltv_list_get(GLTV_LIST, unsigned);
/* reset value at position n (same condition) */
extern void gltv_list_set(GLTV_LIST, unsigned, void *);
/* insert value at position n (same condition) */
extern void gltv_list_insert(GLTV_LIST, unsigned, void *);
/* remove value at position n (same condition) */
extern void *gltv_list_remove(GLTV_LIST, unsigned);

#endif
