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
#include "list.h"
#include "log.h"
#include "memspool.h"
#include "stack.h"
#include <assert.h>

/* no pointers because of reallocation */
struct s_entry {
	unsigned next;	/* next=0 means last, =1 means vaccant */
	unsigned previous;
	void *value;
};
typedef struct s_entry entry;

struct s_gltv_list {
	unsigned nb_usable_entries;
	unsigned nb_used_entries;
	unsigned begin;
	unsigned end;
	unsigned last_accessed_order;	/* order into the list */
	unsigned last_accessed_index;	/* indices are thoses of entries */
	entry *entries;
	unsigned next_free_entry;	/* index of the next free entry in entries */
	unsigned full_until;	/* index of the first entry which usage might be vaccant */
	gltv_stack *removed;	/* indexes of thet lasts removed entries */
	char optimize_for_size;
};
typedef struct s_gltv_list list;

GLTV_LIST gltv_list_new(unsigned total, int optimize_for_size) {
	list *this_list;
	this_list = gltv_memspool_alloc(sizeof(list));
	if (!this_list) return 0;
	this_list->entries = gltv_memspool_alloc(total*sizeof(entry));
	if (! this_list->entries) {
		gltv_memspool_unregister(this_list);
		return 0;
	}
	if (!(this_list->removed = gltv_stack_new(1+(total>>3), !optimize_for_size)) ) {
		gltv_memspool_unregister(this_list->entries);
		gltv_memspool_unregister(this_list);
		return 0;
	}
	this_list->nb_usable_entries = total;
	/* the 0th entry is unusable in practice because when next=0 it means : last entry.
	 * the 1st entry is unusable also because we tag vaccant entries with next=1 */
	this_list->nb_used_entries = 2;
	this_list->begin = 0;	/* should be == entries[0].next */
	this_list->end = 0;
	this_list->last_accessed_index = 0;	/* means none. last_accessed_order is then irrelevant */
	this_list->next_free_entry = 2;
	this_list->full_until = 2;
	this_list->optimize_for_size = optimize_for_size;
	return (GLTV_LIST)this_list;
}

void gltv_list_del(GLTV_LIST l) {
	gltv_memspool_unregister(l->entries);
	gltv_stack_del(l->removed);
	gltv_memspool_unregister(l);
}

static void resize(list *l, unsigned nb_entries) {
	/* resizing is for entries, not lines */
	entry *ptr;
	assert(l->next_free_entry<=nb_entries);
	ptr = (entry*) gltv_memspool_realloc(l->entries, nb_entries*sizeof(entry));
	if (!ptr) gltv_log_fatal("list:resize : can't resize");
	l->entries = ptr;
	l->nb_usable_entries = nb_entries;
}

static unsigned alloc_vaccant_index(list *l) {
	/* trouver un emplacement vide dans entries
	 * y loger la valeur et changer les liens, etc */
	unsigned index;
start:
	if (l->next_free_entry < l->nb_usable_entries) {
		index = l->next_free_entry;
		if (l->full_until == l->next_free_entry) l->full_until ++;
		l->next_free_entry ++;
	} else {
		if (gltv_stack_size(l->removed)) {
			index = (unsigned) gltv_stack_pop(l->removed);
			assert(l->entries[index].next==1);
		} else {
			if (l->nb_used_entries == l->nb_usable_entries) {
				resize(l, (l->nb_usable_entries*3)>>1);
				goto start;
			}
			assert(l->optimize_for_size); /* otherwise we should have had something on the stack */
			for (index=l->full_until; index<l->nb_usable_entries; index++)
				if (1 == l->entries[index].next) {
					l->full_until = index;
					break;
				}
			assert(index<l->nb_usable_entries);
		}
	}
	l->nb_used_entries++;
	return index;
}

static void desalloc_index(GLTV_LIST l, unsigned index) {
	if (index == l->next_free_entry-1) {
		l->next_free_entry--;
	} else {
		if (index < l->full_until)
			l->full_until = index;
		gltv_stack_push(l->removed, (void*)index);
		l->entries[index].next=1;	/* tag as vaccant */
	}
	l->nb_used_entries--;
	if (l->last_accessed_index == index) l->last_accessed_index=0;	
}

int gltv_list_push(GLTV_LIST l, void *value) {
	unsigned index = alloc_vaccant_index(l);
	l->entries[index].value = value;
	l->entries[index].next = 0;
	l->entries[index].previous = l->end;
	l->entries[l->end].next = index;
	l->end = index;
	/* perhaps useless if entries[0].next always equals begin */
	if (1==gltv_list_size(l)) l->begin = index;
	return 1;
}

void *gltv_list_pop(GLTV_LIST l) {
	void *value;
	unsigned new_end;
	assert(l->begin != 0);
	assert(l->entries[l->end].next == 0);
	value = l->entries[l->end].value;
	new_end = l->entries[l->end].previous;
	desalloc_index(l, l->end);
	l->end = new_end;
	l->entries[new_end].next = 0;
	/* perhaps useless if entries[0].next always equals begin */
	if (0==gltv_list_size(l)) l->begin = 0;
	return value;
}

static unsigned order_to_index(GLTV_LIST l, unsigned order) {
	unsigned length = gltv_list_size(l);
	unsigned dend, dlast;
	unsigned index, dexterm, o;
	assert(order<length);
	assert(l->last_accessed_index<l->next_free_entry);
	dend = length-1-order;
	if (order < dend) {
		dexterm = order;
	} else {
		dexterm = dend;
	}
	if (!l->last_accessed_index)
		goto from_extrem;
	dlast = order - l->last_accessed_order;
	if (order < l->last_accessed_order)
		dlast = 0 - dlast;
	if (dexterm < dlast)
		goto from_extrem;
	/* search index from last accessed */
	index = l->last_accessed_index;
	if (order > l->last_accessed_order) {
		while (dlast) {
			index = l->entries[index].next;
			dlast --;
		}
	} else {
		while (dlast) {
			index = l->entries[index].previous;
			dlast --;
		}
	}
	l->last_accessed_order = order;
	l->last_accessed_index = index;
	assert(index>1);
	return index;
from_extrem:
	/* search index from extremums */
	o = dexterm;
	if (order < dend) {	/* from beginning */
		index = l->begin;
		while (o) {
			index = l->entries[index].next;
			o --;
		}
	} else {
		index = l->end;
		while (o) {
			index = l->entries[index].previous;
			o --;
		}
	}
	if (dexterm>5) {
		l->last_accessed_order = order;
		l->last_accessed_index = index;
	}
	assert(index>1);
	return index;
}

void *gltv_list_get(GLTV_LIST l, unsigned order) {
	unsigned index = order_to_index(l, order);
	return l->entries[index].value;
}

void gltv_list_set(GLTV_LIST l, unsigned order, void *value) {
	unsigned index = order_to_index(l, order);
	l->entries[index].value = value;
}

void gltv_list_insert(GLTV_LIST l, unsigned order, void *value) {
	unsigned size = gltv_list_size(l);
	if (order==size) {	/* we asked for a push */
		gltv_list_push(l, value);
	} else {
		unsigned free_idx;
		unsigned index, previous;
		assert(order<=size);
		index = order_to_index(l, order);
		free_idx = alloc_vaccant_index(l);
		previous = l->entries[index].previous;
		l->entries[free_idx].value = value;
		l->entries[free_idx].previous = previous;
		l->entries[free_idx].next = index;
		l->entries[index].previous = free_idx;
		l->entries[previous].next = free_idx;
		/* as we change the order of elements after this one... */
		l->last_accessed_index = free_idx; 
		l->last_accessed_order = order;
		/* perhaps useless if entries[0].next always equals begin */
		if (0==order) l->begin = free_idx;
	}
}

void *gltv_list_remove(GLTV_LIST l, unsigned order) {
	unsigned index = order_to_index(l, order);
	void *value = l->entries[index].value;
	unsigned previous = l->entries[index].previous;
	unsigned next = l->entries[index].next;
	l->entries[previous].next = next;
	l->entries[next].previous = previous;
	desalloc_index(l, index);
	/* as we change the order of elements after this one... */
	l->last_accessed_index = next; 
	l->last_accessed_order = order;
	/* perhaps useless if entries[0].next always equals begin */
	if (index==l->begin) l->begin = next;
	if (index==l->end) l->end = previous;
	return value;
}

void gltv_list_clear(GLTV_LIST l) {
	gltv_stack_clear(l->removed);
	l->nb_used_entries = 2;
	l->begin = 0;	/* should be == entries[0].next */
	l->end = 0;
	l->last_accessed_index = 0;	/* means none. last_accessed_order is then irrelevant */
	l->next_free_entry = 2;
	l->full_until = 2;
}

unsigned gltv_list_size(GLTV_LIST l) {
	return l->nb_used_entries -2;
}

