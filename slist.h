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
 * Functions to create and manipulate a sortted list of void*.
 */

#ifndef _GLTV_SLIST_
#define _GLTV_SLIST_

struct s_gltv_slist;
typedef struct s_gltv_slist* GLTV_SLIST;

/* to get a new empty list */
/* size is the total estimated max size of the slist.
 * if resizeable, the slist will expand as necessary
 * comp is the comparison function, return -1, 0 or 1 to tell wether its first arg is lower, equal or greater than its second
 */
extern GLTV_SLIST gltv_slist_new(unsigned, int, int (*)(void *, void *));
/* to delete a slist */
extern void gltv_slist_del(GLTV_SLIST);
/* to empty the slist (the slist keeps its current size, even if resized) */
extern void gltv_slist_clear(GLTV_SLIST);
/* tells the list size */
extern unsigned gltv_slist_size(GLTV_SLIST);
/* returns value n (optimised if last accessed value was not far from n) */
extern void *gltv_slist_get(GLTV_SLIST, unsigned);
/* reset value at position n (same condition) */
extern void gltv_slist_set(GLTV_SLIST, unsigned, void *);
/* remove value at position n (same condition) */
extern void *gltv_slist_remove(GLTV_SLIST, unsigned);
/* insert value (returns the index where the value was inserted) */
extern unsigned gltv_slist_insert(GLTV_SLIST, void *);

#endif
