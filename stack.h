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
 * Functions to create and manipulate a stack of void*.
 */

#ifndef _GLTV_STACK_
#define _GLTV_STACK_

struct s_gltv_stack;
typedef struct s_gltv_stack gltv_stack;

/* to get a new empty stack */
/* size is the total estimated max size of the stack.
 * if resizeable, the stack will expand as necessary */
extern gltv_stack *gltv_stack_new(unsigned, char);
/* to delete a stack */
extern void gltv_stack_del(gltv_stack *);
/* to push a value */
extern int gltv_stack_push(gltv_stack *, void *);
/* to pop a value */
extern void *gltv_stack_pop(gltv_stack *);
/* to empty the stack (the stack keeps its current size, even if resized) */
extern void gltv_stack_clear(gltv_stack *);
/* tells the stack size */
extern unsigned gltv_stack_size(gltv_stack *);

#endif
