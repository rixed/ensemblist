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
#include "hash.h"
#include "stack.h"
#include "memspool.h"
#include "log.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct {
	unsigned key;
	void* value;
	unsigned next;	/* 0 if last, 1 if vaccant */
} entry;

typedef struct {
	unsigned first;	/* first and next are indices cause realloc may relocate the entries */
} hash_line;

struct s_gltv_hash {
	entry* entries;
	unsigned nb_used_entries;
	unsigned nb_lines;
	unsigned nb_usable_entries;
	unsigned next_free_entry;	/* index of the next free entry in entries */
	unsigned full_until;	/* index of the first entry which usage might be vaccant */
	char flag;
	unsigned read_next;
	gltv_stack *removed;	/* indexes of thet lasts removed entries */
	hash_line lines[0];	/* GCC specific */
};

typedef struct s_gltv_hash hash;

GLTV_HASH gltv_hash_new(unsigned total, unsigned per_line, int flag) {
	unsigned nb_lines, i;
	entry *entries;
	hash *this_hash;
	assert(total && per_line);
	nb_lines = (3*(total/per_line))>>1;
	this_hash = gltv_memspool_alloc(sizeof(hash)+nb_lines*sizeof(hash_line));
	if (!this_hash) return 0;
	entries = gltv_memspool_alloc(total*sizeof(entry));
	if (!entries) {
		gltv_memspool_unregister(this_hash);
		return 0;
	}
	if (!(this_hash->removed = gltv_stack_new(1+(total>>3), flag&GLTV_HASH_OPT_SPEED)) ) {
		gltv_memspool_unregister(entries);
		gltv_memspool_unregister(this_hash);
		return 0;
	}
	this_hash->nb_lines = nb_lines;
	this_hash->entries = entries;
	this_hash->nb_usable_entries = total;
	/* the 0th entry is unusable in practice because when next=0 it means : last entry.
	 * the 1st entry is unusable also because we tag vaccant entries with next=1 */
	this_hash->nb_used_entries = 2;
	this_hash->next_free_entry = 2;
	this_hash->full_until = 2;
	this_hash->flag = flag;
	for (i=0; i<nb_lines; i++) {
		this_hash->lines[i].first = 0;
	}
	return (GLTV_HASH)this_hash;
}

void gltv_hash_del(GLTV_HASH h) {
	gltv_memspool_unregister(h->entries);
	gltv_stack_del(h->removed);
	gltv_memspool_unregister(h);
}

static unsigned hash_func(GLTV_HASH hash, unsigned key) {
	unsigned index = 0;
	unsigned c;
	if (hash->flag & GLTV_HASH_STRKEYS) {
		char *str = (char*)key;
		c = 0;
		while (str[c]!='\0') {	/* we could limit c to, say, 10 chars, but if the string is a full pathname, it wont be smart. in the opposite direction, same problem */
			index += 31 + str[c++];
		}
	} else {
		unsigned k = key;
		for (c=0; c<4; c++) {
			index += 31 + (k&0xFF);
			k >>= 8;
		}
	}
	index %= hash->nb_lines;
	return index;
}

int keysMatch(GLTV_HASH hash, unsigned k1, unsigned k2) {
	if (hash->flag & GLTV_HASH_STRKEYS)
		return 0==strcmp((char*)k1, (char*)k2);
	else
		return k1==k2;
}

static void resize(hash *h, unsigned nb_entries) {
	/* resizing if for entries, not lines */
	entry *ptr;
	assert(h->next_free_entry<=nb_entries);
	ptr = (entry*) gltv_memspool_realloc(h->entries, nb_entries*sizeof(entry));
	if (!ptr) gltv_log_fatal("hash:resize : can't resize");
	h->entries = ptr;
	h->nb_usable_entries = nb_entries;
}

void gltv_hash_put(GLTV_HASH h, unsigned key, void *value) {
	unsigned line = hash_func(h, key);
	unsigned *eptr_location, eptr, old_next;
start:
	eptr_location = &h->lines[line].first;
	while ( (eptr = *eptr_location) !=0) {
		assert(eptr!=1);
		if (keysMatch(h, h->entries[eptr].key, key)) {	/* update */
			h->entries[eptr].value = value;
			h->entries[eptr].key = key;
			return;
		}
		eptr_location = &h->entries[eptr].next;
	}
	/* insert */
	if (h->next_free_entry < h->nb_usable_entries) {
		/* some space left at the end */
		eptr = h->next_free_entry;
		if (h->full_until == h->next_free_entry) h->full_until ++;
		h->next_free_entry ++;
	} else {
		/* no easy space */
		/* try to use the stack of removed entries to store vacant locations. */
		if (gltv_stack_size(h->removed)) {
			eptr = (unsigned) gltv_stack_pop(h->removed);
			assert(h->entries[eptr].next==1);
		} else {
			/* otherwise parse everything to find vacant entries
			 * (this assumes that we MARK removed entries) */
			if (h->nb_used_entries == h->nb_usable_entries) {
				/* no need for parsing, resize the hash */
				resize(h, (h->nb_usable_entries*3)>>1);
				/* some new space left at the end
				 * but eptr_location, beeing a pointer, must be discarded
				 * so let's start over */
				goto start;
			} else {
				assert((h->flag&GLTV_HASH_OPT_SPEED) == 0);
				/* we have a hint from where to parse */
				for (eptr=h->full_until; eptr<h->nb_usable_entries; eptr++)
					if (1 == (int)h->entries[eptr].next) {
						h->full_until = eptr;
						break;
					}
				assert(eptr<h->nb_usable_entries);
			}
		}
	}
	/* common ending for the insert cases */
	old_next = *eptr_location;
	*eptr_location = eptr;
	h->entries[eptr].next = old_next;
	h->entries[eptr].key = key;
	h->entries[eptr].value = value;
	h->nb_used_entries++;
}

char gltv_hash_get(GLTV_HASH h, unsigned key, void **value) {
	unsigned line = hash_func(h, key);
	unsigned eptr = h->lines[line].first;
	while ( eptr != 0) {
		if (keysMatch(h, h->entries[eptr].key, key)) {
			*value = h->entries[eptr].value;
			return 1;
		}
		eptr = h->entries[eptr].next;
	}
	/* asking for an inexistent key */
	return 0;
}

char gltv_hash_remove(GLTV_HASH h, unsigned key) {
	unsigned line = hash_func(h, key);
	unsigned eptr = h->lines[line].first;
	unsigned old_eptr = 0;
	while ( eptr != 0) {
		if (keysMatch(h, h->entries[eptr].key, key)) {
			gltv_stack_push(h->removed, (void*)eptr);
			if (0 != old_eptr)
				h->entries[old_eptr].next = h->entries[eptr].next;
			else
				h->lines[line].first = h->entries[eptr].next;
			if (eptr < h->full_until)
				h->full_until = eptr;
			h->entries[eptr].next=1;	/* tag as vaccant */
			h->nb_used_entries--;
			return 1;
		}
		old_eptr = eptr;
		eptr = h->entries[eptr].next;
	}
	return 0;
}

unsigned gltv_hash_size(GLTV_HASH h) {
	return h->nb_used_entries -2;
}

void gltv_hash_reset(GLTV_HASH h) {
	h->read_next = 2;
}

char gltv_hash_each(GLTV_HASH h, unsigned *key, void **value) {
	unsigned i;
	for (i=h->read_next; i<h->next_free_entry; i++) {
		if (1 != h->entries[i].next) {
			*key = h->entries[i].key;
			*value = h->entries[i].value;
			h->read_next = i+1;
			return 1;
		}
	}
	return 0;
}

void gltv_hash_compact(GLTV_HASH h) {
	unsigned i, line;
	for (i=h->full_until; i<h->next_free_entry; i++) {
		if (h->entries[i].next==1) {
			/* replace this entry with the last entry used */
			unsigned ii, eptr;
			unsigned *eptr_location;
			for (ii=h->next_free_entry-1; ii>i; ii--)
				if (h->entries[ii].next!=1) break;
			if (ii==i) goto near_end;
			line = hash_func(h, h->entries[ii].key);
			eptr_location = &h->lines[line].first;
			while ( (eptr=*eptr_location) != ii && eptr != 0) {
				eptr_location = &h->entries[eptr].next;
				eptr = *eptr_location;
			}
			assert(eptr==ii);	/* So, we could simplify the while clause */
			*eptr_location = i;
			h->entries[i].key = h->entries[ii].key;
			h->entries[i].value = h->entries[ii].value;
			h->entries[i].next = h->entries[ii].next;
near_end:
			h->next_free_entry = ii;
		}
	}
	h->full_until = h->next_free_entry;
	resize(h, h->nb_used_entries);
}

