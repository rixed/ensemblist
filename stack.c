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
#include "stack.h"
#include "log.h"
#include "memspool.h"
#include <assert.h>
#define STR(x) #x
#define XSTR(x) STR(x)

struct s_gltv_stack {
	unsigned next;
	unsigned max_size;
	char resizeable;
	intptr_t *entries;
};

/* TODO add stack_compact and use it if resizeable in pop and clear to match initial size */

gltv_stack *gltv_stack_new(unsigned max_size, char resizeable) {
	gltv_stack *this_stack;
	this_stack = gltv_memspool_alloc(sizeof(*this_stack));
	if (!this_stack) return 0;
	this_stack->entries = gltv_memspool_alloc(max_size*sizeof(*this_stack->entries));
	if (!this_stack->entries) {
		gltv_memspool_unregister(this_stack);
		return 0;
	}
	this_stack->max_size = max_size;
	this_stack->next = 0;
	this_stack->resizeable = resizeable;
	return this_stack;
}

void gltv_stack_del(gltv_stack *s) {
	gltv_memspool_unregister(s->entries);
	gltv_memspool_unregister(s);
}

int gltv_stack_push(gltv_stack *s, intptr_t value) {
	assert(s->next<=s->max_size);
	if (s->next == s->max_size) {
		if (s->resizeable) {
			s->max_size <<= 1;
			if (!(s->entries = gltv_memspool_realloc(s->entries, s->max_size*sizeof(*s->entries))) )
				gltv_log_fatal(XSTR(__FILE__) XSTR(__LINE__) "Can't resize stack");
		} else {
			return 0;
		}
	}
	s->entries[s->next++] = value;
	return 1;
}

intptr_t gltv_stack_pop(gltv_stack *s) {
	assert(s->next>0);
	return s->entries[--s->next];
}

void gltv_stack_clear(gltv_stack *s) {
	s->next = 0;
}

unsigned gltv_stack_size(gltv_stack *s) {
	return s->next;
}

