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
#include "slist.h"
#include "list.h"
#include "log.h"
#include "memspool.h"
#include "stack.h"
#include <assert.h>

/* heavy use of list */

struct s_gltv_slist {
	GLTV_LIST list;
	int (*comp)(void *, void *);
};
typedef struct s_gltv_slist slist;

GLTV_SLIST gltv_slist_new(unsigned total, int optimize_for_size, int (*comp)(void *, void *)) {
	slist *this_slist;
	this_slist = gltv_memspool_alloc(sizeof(slist));
	if (!this_slist) return 0;
	this_slist->list = gltv_list_new(total, optimize_for_size);
	if (! this_slist->list) {
		gltv_memspool_unregister(this_slist);
		return 0;
	}
	this_slist->comp = comp;
	return (GLTV_SLIST)this_slist;
}

void gltv_slist_del(GLTV_SLIST l) {
	gltv_list_del(l->list);
	gltv_memspool_unregister(l);
}

void gltv_slist_clear(GLTV_SLIST l) {
	gltv_list_clear(l->list);
}

unsigned gltv_slist_size(GLTV_SLIST l) {
	return gltv_list_size(l->list);
}

void *gltv_slist_get(GLTV_SLIST l, unsigned order) {
	return gltv_list_get(l->list, order);
}

/* to keep ordering, we remove then insert back */
void gltv_slist_set(GLTV_SLIST l, unsigned order, void *value) {
	gltv_slist_remove(l, order);
	gltv_slist_insert(l, value);
}

void *gltv_slist_remove(GLTV_SLIST l, unsigned order) {
	return gltv_list_remove(l->list, order);
}

/* all the real work is done at insert */
unsigned gltv_slist_insert(GLTV_SLIST l, void *value) {
	unsigned size = gltv_list_size(l->list);
	unsigned index;
	for (index=0; index<size; index++) {
		int c = l->comp(gltv_list_get(l->list, index), value);
		if (c>=0) break;
	}
	gltv_list_insert(l->list, index, value);
	return index;
}

