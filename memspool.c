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
#include "memspool.h"
#include "log.h"
#include "slist.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct head {
	unsigned mark;
	unsigned size;
	unsigned ref_count;
	struct head *next, *prec;
	char const *caller_file;
   size_t caller_line;
	char block[0];
};

static unsigned size=0;
static unsigned used;
static struct head *first, *last;

int gltv_memspool_init(unsigned new_size)
{
	assert(size==0);
	used = 0;
	size = new_size;
	first = last = NULL;
	return 1;
}

void gltv_memspool_end()
{
	if (used) {
		struct head *ptr = first;
		gltv_log_warning(GLTV_LOG_MUSTSEE, "memspool_end: some memory left but asking to end !");
		while (ptr!=last) {
			gltv_log_warning(GLTV_LOG_MUSTSEE, "memspool_end: still %u at %p for %s:%u", ptr->size, ptr, ptr->caller_file, ptr->caller_line);
			ptr = ptr->next;
		};
		gltv_log_warning(GLTV_LOG_MUSTSEE, "memspool_end: still %u at %p for %s:%u", ptr->size, ptr, ptr->caller_file, ptr->caller_line);
	}
	size = 0;
}

unsigned gltv_memspool_blocksize(void *mem)
{
	assert(mem && size);
	mem = (char*)mem - sizeof(struct head);
	return ((struct head*)mem)->size;
}

unsigned gltv_memspool_refcount(void *mem)
{
	assert(mem && size);
	mem = (char*)mem - sizeof(struct head);
	return ((struct head*)mem)->ref_count;
}

char gltv_memspool_isvalid(void *mem)
{
	assert(mem && size);
	mem = (char*)mem - sizeof(struct head);
	return ((struct head*)mem)->mark==0x280176;
}

void *gltv_memspool_alloc_d(unsigned requested_size, char const *file, size_t line)
{
	char *mem;
	assert(size);
	mem = malloc(requested_size+sizeof(struct head));
	if (mem) {
		if (!first) {
			first = (struct head*)mem;
		} else {
			((struct head*)last)->next = (struct head*)mem;
		}
		((struct head*)mem)->mark = 0x280176;
		((struct head*)mem)->size = requested_size;
		((struct head*)mem)->ref_count = 1;
		((struct head*)mem)->prec = last;
		((struct head*)mem)->next = 0;
		((struct head*)mem)->caller_file = file;
		((struct head*)mem)->caller_line = line;
		last = (struct head*)mem;
		gltv_log_warning(GLTV_LOG_DEBUG, "memspool: alloc %d bytes at %p (used = %d) for %s:%u", requested_size, mem, used, file, line);
		mem += sizeof(struct head);
		used += requested_size;
	}
	return (void*)mem;
}

void *gltv_memspool_realloc(void *mem, unsigned new_size)
{
	char *ptr;
	struct head *p2;
	assert(size);
	if (!mem) {
		return gltv_memspool_alloc(new_size);
	}
	assert(gltv_memspool_isvalid(mem));
	mem = (char*)mem - sizeof(struct head);
	ptr = realloc(mem, new_size+sizeof(struct head));
	if (!ptr) return 0;
	used += (new_size - ((struct head*)ptr)->size);
	((struct head*)ptr)->size = new_size;
	if (mem!=first) {
		p2 = ((struct head*)ptr)->prec;
		assert(p2->mark==0x280176);
		p2->next = (struct head*)ptr;
	} else {
		first = (struct head*)ptr;
	}
	if (mem!=last) {
		p2 = ((struct head*)ptr)->next;
		assert(p2->mark==0x280176);
		p2->prec = (struct head*)ptr;
	} else {
		last = (struct head*)ptr;
	}
	gltv_log_warning(GLTV_LOG_DEBUG, "memspool: realloc %p at %p for %d bytes (used = %d)", mem, ptr, new_size, used);
	return ptr+sizeof(struct head);
}

void gltv_memspool_unregister(void *mem)
{
	struct head *p2;
	assert(size && mem);
	assert(gltv_memspool_isvalid(mem));
	mem = (char*)mem - sizeof(struct head);
	if (0 == --((struct head*)mem)->ref_count) {
		used -= ((struct head*)mem)->size;
		if (mem!=first) {
			p2 = ((struct head*)mem)->prec;
			assert(p2->mark==0x280176);
			p2->next = ((struct head*)mem)->next;
		} else {
			first = ((struct head*)mem)->next;
		}
		if (mem!=last) {
			p2 = ((struct head*)mem)->next;
			assert(p2->mark==0x280176);
			p2->prec = ((struct head*)mem)->prec;
		} else {
			last = ((struct head*)mem)->prec;
		}
		gltv_log_warning(GLTV_LOG_DEBUG, "memspool: free at %p (used = %d)", mem, used);
#ifndef NDEBUG
		memset(((struct head*)mem)->block, 0xce, ((struct head*)mem)->size);
#endif
		free(mem);
	}
}

int gltv_memspool_register(void *mem)
{
	/* mem MUST be the adress of the first byte of a block */
	assert(size && mem);
	assert(gltv_memspool_isvalid(mem));
	mem = (char*)mem - sizeof(struct head);
	((struct head*)mem)->ref_count++;
	return 1;
}

int gltv_memspool_consumption()
{
	assert(size);
	return (100L*used)/size;
}

void gltv_memspool_print()
{
	assert(size);
	if (!first) {
		gltv_log_warning(GLTV_LOG_MUSTSEE, "gltv_memspool_print: nothing");
	} else {
		struct head *ptr = first;
		do {
			assert(ptr->mark == 0x280176);
			gltv_log_warning(GLTV_LOG_MUSTSEE, "gltv_memspool_print : %u bytes at %p, created by %s:%u, referenced %u time(s)", ptr->size, ptr, ptr->caller_file, ptr->caller_line, ptr->ref_count);
			if (ptr==last) break;
			ptr = ptr->next;
		} while (1);
	}
}
