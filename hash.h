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
 * Functions to create and manipulate a hash.
 * The key is an unsigned, the value a void*.
 *
 */

#ifndef _GLTV_HASH_
#define _GLTV_HASH_

#define GLTV_HASH_OPT_SIZE 0x0
#define GLTV_HASH_OPT_SPEED 0x1
#define GLTV_HASH_STRKEYS 0x2

struct s_gltv_hash;
typedef struct s_gltv_hash* GLTV_HASH;

/* to get a new empty hash.
 * nbr_of_total_elements is the expected total numbers of elements
 * nbr_of_elements_per_line is an accepted hash collision per key (better case)
 * if GLTV_HASH_OPT_SIZE the stack of remove entries is smaller
 * if GLTV_HASH_OPT_SPEED the stack will expand as necessary
 * if GLTV_HASH_STRKEYS the passed keys are, in fact, char* of strings...
 * Beware, in this case, that you are responsible for keeping this pointer valid
 * for the whole life of the hash. in a next version, the hash could register for this memory
 * and release it when the entry is deleted...
 */

extern GLTV_HASH gltv_hash_new(unsigned, unsigned, int);
/* to delete a hash */
extern void gltv_hash_del(GLTV_HASH);
/* to put a new pair of key/value or update the value associated with the key */
/* Its not guaranted weither a given implementation would permit to convert a pointer to an int and back again to the same pointer. It MUST works, otherwise take long or whatever for holding keys */
extern void gltv_hash_put(GLTV_HASH, unsigned, void *);
/* to get the value associated with a key (in value). return 0 if key does not exists */
extern char gltv_hash_get(GLTV_HASH, unsigned, void **);
/* tells wether a key is defined in the hash */
extern char gltv_hash_defined(GLTV_HASH, unsigned);
/* remove the value associated to the key */
extern char gltv_hash_remove(GLTV_HASH, unsigned);
/* tells how many keys are defined */
extern unsigned gltv_hash_size(GLTV_HASH);
/* reset the read pointer */
extern void gltv_hash_reset(GLTV_HASH);
/* read next pair (key,value). return 0 if there are no more values */
extern char gltv_hash_each(GLTV_HASH, unsigned *, void **);
/* once you are done inserting new key in the hash, you can compact it with
 * the hash still works as a hash, anyway */
extern void gltv_hash_compact(GLTV_HASH);

/* TODO : add a cursor to scan the hash without semaphore ? */

#endif
