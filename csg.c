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
#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>
#include "csg.h"
#include "memspool.h"
#include "geom.h"
#include "log.h"

const char operator[] = { 'X', '/', '|', '&' };

static char *substr(char *s, unsigned l)
{
	static char str[NB_MAX_PRIMS_IN_TREE*3];
	memcpy(str, s, l);
	str[l] = '\0';
	return str;
}

void csg_print_tree(csg_node *node)
{
	if (NULL==node) {
		printf("csg_print_tree: Cannot print NULL tree\n");
		return;
	}
	switch (node->type) {
		case CSG_STRING:
			printf("Type String : '%s'\n", substr(node->u.s.string, node->u.s.len));
			break;
		case CSG_PRIM:
			printf("Prim(%s)\n", node->u.prim.m->name);
			break;
		case CSG_OP:
			printf("Operator %c\n", operator[node->u.op.op]);
			printf("Left :\n");
			csg_print_tree(node->u.op.left);
			printf("Right :\n");
			csg_print_tree(node->u.op.right);
			break;
	}
	return;
}

static csg_node *csg_node_new(char *string, unsigned len)
{
	csg_node *node = gltv_memspool_alloc(sizeof(*node));
	gltv_log_warning(GLTV_LOG_DEBUG, "creation of a node at %p\n", node);
	if (!node) return NULL;
	node->type = CSG_STRING;
	node->u.s.string = string;
	node->u.s.len = len;
	return node;
}

static void csg_node_del_this_one_only(csg_node *node)
{
	gltv_log_warning(GLTV_LOG_DEBUG, "deletion of a node at %p\n", node);
	gltv_memspool_unregister(node);
}

void csg_node_del(csg_node *node)
{
	if (node->type == CSG_OP) {
		if (NULL != node->u.op.left) csg_node_del(node->u.op.left);
		if (NULL != node->u.op.right) csg_node_del(node->u.op.right);
	}
	csg_node_del_this_one_only(node);
}

static csg_node *csg_node_copy(csg_node *orig)
{
	csg_node *copy = csg_node_new(NULL, 0);
	if (NULL == copy) return NULL;
	memcpy(copy, orig, sizeof(*orig));
	if (CSG_OP == copy->type) {
		copy->u.op.left = csg_node_copy(orig->u.op.left);
		copy->u.op.right = csg_node_copy(orig->u.op.right);
		if (NULL==copy->u.op.left || NULL==copy->u.op.right) {
			csg_node_del(copy);
			return NULL;
		}
	}
	return copy;
}

static int csg_string_parenth_ok(char *str, unsigned len)
{
	unsigned i;
	int p_count;
	for (i=0, p_count=0; i<len; i++) {
		if (str[i]=='(') p_count++;
		else if (str[i]==')') p_count--;
		if (p_count<0) return 0;
	}
	return p_count==0;
}

static int csg_parse(csg_node *node, unsigned nb_prims, mesh **meshes)
{
	unsigned len;
	char *str;
	int p_count;
	int c;
	if (node->type!=CSG_STRING) return 1;
	str = node->u.s.string;
	len = node->u.s.len;
	/* verifs */
	if (len==0) {
		gltv_log_warning(GLTV_LOG_OPTIONAL, "Bad string '%s'\n",substr(str,len));
		return 0;
	}
	if (!csg_string_parenth_ok(str, len)) {
		gltv_log_warning(GLTV_LOG_OPTIONAL, "Bad parenthesis in '%s'\n", substr(str,len));
		return 0;
	}
	/* virer éventuelement des parenthèses extérieures */
	if (str[0]=='(' && str[len-1]==')' && csg_string_parenth_ok(str+1,len-2)) {
		node->u.s.string ++;
		node->u.s.len -= 2;
		return csg_parse(node, nb_prims, meshes);
	}
	/* chercher le dernier opérateur extérieur - so that operators are all left associative */
	for (c=len-1, p_count=0; c>=0 && (p_count!=0 || (str[c]!=operator[CSG_MINUS] && str[c]!=operator[CSG_UNION] && str[c]!=operator[CSG_AND])); c--) {
		if (str[c]=='(') p_count++;
		else if (str[c]==')') p_count--;
	}
	if (c>0) {
		csg_op op;
		if (str[c]==operator[CSG_MINUS]) {
			op = CSG_MINUS;
		} else if (str[c]==operator[CSG_UNION]) {
			op = CSG_UNION;
		} else if (str[c]==operator[CSG_AND]) {
			op = CSG_AND;
		}
		node->type = CSG_OP;
		node->u.op.op = op;
		node->u.op.left = csg_node_new(str, c);
		node->u.op.right = csg_node_new(str+c+1, len-c-1);
		return csg_parse(node->u.op.left, nb_prims, meshes) && csg_parse(node->u.op.right, nb_prims, meshes);
	} else {
		/* pas d'op extérieur, on a donc normalement une primitive seule */
		int i;
		/* lit le No de la primitive */
		i = str[0]-'a';
		if (i<0 || (unsigned)i>=nb_prims) {
			gltv_log_fatal("csg_parse: Unknown primitive : '%d'", i);
		}
		node->type = CSG_PRIM;
		node->u.prim.m = meshes[i];
		if (len>1) {
			gltv_log_fatal("csg_parse: pending chars : '%s'\n", str+1);
		} else {
			return 1;
		}
	}
}

static int csg_normalize_node(csg_node *node)
{
	csg_op op, opl, opr;
	csg_node *x, *y, *z;
	op = node->u.op.op;
	opr = CSG_NOOP;
	if (node->u.op.right->type == CSG_OP) opr = node->u.op.right->u.op.op;
	/* rule 1 : X-(Y|Z) -> (X-Y)-Z */
	if (op==CSG_MINUS && opr==CSG_UNION) {
		x = node->u.op.left;
		y = node->u.op.right->u.op.left;
		z = node->u.op.right->u.op.right;
		node->u.op.left = node->u.op.right;
		node->u.op.left->u.op.op = CSG_MINUS;
		node->u.op.left->u.op.left = x;
		node->u.op.left->u.op.right = y;
		node->u.op.right = z;
		return 1;
	}
	/* rule 2 : X&(Y|Z) -> (X&Y)|(X&Z) */
	if (op==CSG_AND && opr==CSG_UNION) {
		csg_node *new_node = csg_node_new(NULL, 0);
		if (NULL==new_node) return 2;
		x = node->u.op.left;
		y = node->u.op.right->u.op.left;
		z = node->u.op.right->u.op.right;
		node->u.op.op = CSG_UNION;
		node->u.op.left = new_node;
		node->u.op.right->u.op.op = CSG_AND;
		if ((node->u.op.right->u.op.left = csg_node_copy(x))==NULL) return 2;
		new_node->type = CSG_OP;
		new_node->u.op.op = CSG_AND;
		new_node->u.op.left = x;
		new_node->u.op.right = y;
		return 1;
	}
	/* rule 3 : X-(Y&Z) -> (X-Y)|(X-Z) */
	if (op==CSG_MINUS && opr==CSG_AND) {
		csg_node *new_node = csg_node_new(NULL, 0);
		if (NULL==new_node) return 2;
		x = node->u.op.left;
		y = node->u.op.right->u.op.left;
		z = node->u.op.right->u.op.right;
		node->u.op.op = CSG_UNION;
		node->u.op.left = new_node;
		node->u.op.right->u.op.op = CSG_MINUS;
		if ((node->u.op.right->u.op.left = csg_node_copy(x))==NULL) return 2;
		new_node->type = CSG_OP;
		new_node->u.op.op = CSG_MINUS;
		new_node->u.op.left = x;
		new_node->u.op.right = y;
		return 1;
	}
	/* rule 4 : X&(Y&Z) -> (X&Y)&Z */
	if (op==CSG_AND && opr==CSG_AND) {
		csg_node *tmp = node->u.op.left;
		x = node->u.op.left;
		y = node->u.op.right->u.op.left;
		z = node->u.op.right->u.op.right;
		node->u.op.left = node->u.op.right;
		node->u.op.right = tmp;
		node->u.op.left->u.op.left = x;
		node->u.op.left->u.op.right = y;
		node->u.op.right = z;
		return 1;
	}
	/* rule 5 : X-(Y-Z) -> (X-Y)|(X&Z) */
	if (op==CSG_MINUS && opr==CSG_MINUS) {
		csg_node *new_node = csg_node_new(NULL, 0);
		if (NULL==new_node) return 2;
		x = node->u.op.left;
		y = node->u.op.right->u.op.left;
		z = node->u.op.right->u.op.right;
		node->u.op.op = CSG_UNION;
		node->u.op.left = new_node;
		node->u.op.right->u.op.op = CSG_AND;
		if ((node->u.op.right->u.op.left = csg_node_copy(x))==NULL) return 2;
		new_node->type = CSG_OP;
		new_node->u.op.op = CSG_MINUS;
		new_node->u.op.left = x;
		new_node->u.op.right = y;
		return 1;
	}
	/* rule 6 : X&(Y-Z) -> (X&Y)-Z */
	if (op==CSG_AND && opr==CSG_MINUS) {
		csg_node *tmp = node->u.op.left;
		x = node->u.op.left;
		y = node->u.op.right->u.op.left;
		z = node->u.op.right->u.op.right;
		node->u.op.left = node->u.op.right;
		node->u.op.right = tmp;
		node->u.op.left->u.op.left = x;
		node->u.op.left->u.op.right = y;
		node->u.op.right = z;
		node->u.op.op = CSG_MINUS;
		node->u.op.left->u.op.op = CSG_AND;
		return 1;
	}
	/* rule 7 : (X-Y)&Z -> (X&Z)-Y */
	opl = CSG_NOOP;
	if (node->u.op.left->type == CSG_OP) opl = node->u.op.left->u.op.op;
	if (op==CSG_AND && opl==CSG_MINUS) {
		y = node->u.op.left->u.op.right;
		z = node->u.op.right;
		node->u.op.op = CSG_MINUS;
		node->u.op.left->u.op.op = CSG_AND;
		node->u.op.left->u.op.right = z;
		node->u.op.right = y;
		return 1;
	}
	/* rule 8 : (X|Y)-Z -> (X-Z)|(Y-Z) */
	if (op==CSG_MINUS && opl==CSG_UNION) {
		csg_node *new_node = csg_node_new(NULL, 0);
		if (NULL==new_node) return 2;
		x = node->u.op.left->u.op.left;
		y = node->u.op.left->u.op.right;
		z = node->u.op.right;
		node->u.op.op = CSG_UNION;
		node->u.op.right = new_node;
		node->u.op.left->u.op.op = CSG_MINUS;
		if ((node->u.op.left->u.op.right = csg_node_copy(z))==NULL) return 2;
		new_node->type = CSG_OP;
		new_node->u.op.op = CSG_MINUS;
		new_node->u.op.left = y;
		new_node->u.op.right = z;
		return 1;
	}
	/* rule 9 : (X|Y)&Z -> (X&Z)|(Y&Z) */
	if (op==CSG_AND && opl==CSG_UNION) {
		csg_node *new_node = csg_node_new(NULL, 0);
		if (NULL==new_node) return 2;
		x = node->u.op.left->u.op.left;
		y = node->u.op.left->u.op.right;
		z = node->u.op.right;
		node->u.op.op = CSG_UNION;
		node->u.op.right = new_node;
		node->u.op.left->u.op.op = CSG_AND;
		if ((node->u.op.left->u.op.right = csg_node_copy(z))==NULL) return 2;
		new_node->type = CSG_OP;
		new_node->u.op.op = CSG_AND;
		new_node->u.op.left = y;
		new_node->u.op.right = z;
		return 1;
	}
	return 0;
}

static int csg_normalize_tree(csg_node *node)
{
	int ret;
	if (CSG_OP != node->type) return 1;
	do {
		while ((ret=csg_normalize_node(node))==1);
		if (ret==2) return 0;
		if (!csg_normalize_tree(node->u.op.left)) return 0;
	} while (!(node->u.op.op==CSG_UNION || (node->u.op.right->type==CSG_PRIM && !(node->u.op.left->type==CSG_OP && node->u.op.left->u.op.op==CSG_UNION))));
	return csg_normalize_tree(node->u.op.right);
}

void csg_get_b_box(csg_node *node, b_box *dest_bb)
{
	/* compute the b_box of a node, even a non-terminal one */
	if (node==NULL) {
		dest_bb->vide = 1;
		return;
	}
	if (CSG_PRIM == node->type) {
		/* cas facile */
		geom_get_b_box(node, dest_bb);
		return;
	} else if (CSG_OP == node->type) {
		b_box bb_left, bb_right;
		csg_get_b_box(node->u.op.left, &bb_left);
		csg_get_b_box(node->u.op.right, &bb_right);
		switch (node->u.op.op) {
			case CSG_UNION:
				geom_b_box_union(dest_bb, &bb_left, &bb_right);
				break;
			case CSG_AND:
				geom_b_box_intersection(dest_bb, &bb_left, &bb_right);
				break;
			case CSG_MINUS:
				memcpy(dest_bb, &bb_left, sizeof(*dest_bb));
				break;
			default:
				gltv_log_fatal("csg_get_b_box: unknown csg_op");
				break;
		}
		return;
	} else {
		gltv_log_fatal("csg_get_b_box: call for an invalid csg_node type");
	}
}

static csg_node *csg_prune_tree(csg_node *node)
{
	/* toute opération - ou & de deux primitives sans point commun est inutile */
	char recurs = 0;
	if (CSG_OP != node->type) return node;
	if (CSG_UNION == node->u.op.op) {
		recurs = 1;
	} else {
		b_box p_left, p_right;
		csg_get_b_box(node->u.op.left, &p_left);
		csg_get_b_box(node->u.op.right, &p_right);
		if (geom_b_box_intersects(&p_left, &p_right)) {
			recurs = 1;
		} else {
			if (CSG_AND == node->u.op.op) {
				csg_node_del(node);
				return NULL;
			} else {
				/* MINUS : discard right, promote left */
				csg_node *temp = node->u.op.left;
				csg_node_del(node->u.op.right);
				csg_node_del_this_one_only(node);
				return temp;
			}
		}
	}
	if (recurs) {
		node->u.op.left = csg_prune_tree(node->u.op.left);
		if (NULL==node->u.op.left) {
			if ((CSG_AND==node->u.op.op) || (CSG_MINUS==node->u.op.op)) {
				/* discard the whole node */
				csg_node_del(node);
				return NULL;
			} else {
				/* union : discard this node and promote right */
				csg_node *temp = node->u.op.right;
				csg_node_del_this_one_only(node);
				return csg_prune_tree(temp);
			}
		}
		node->u.op.right = csg_prune_tree(node->u.op.right);
		if (NULL==node->u.op.right) {
			if ((CSG_AND==node->u.op.op)) {
				/* discard the whole node */
				csg_node_del(node);
				return NULL;
			} else {
				/* union or minus : discard this node and promote left */
				csg_node *temp = node->u.op.left;
				csg_node_del_this_one_only(node);
				return temp;
			}
		}
	}
	return node;
}

csg_node *csg_build_tree(char *string, unsigned nb_prims, mesh **meshes)
{
	csg_node *node = csg_node_new(string, strlen(string));
	if (NULL==node) return NULL;
	if (!csg_parse(node, nb_prims, meshes)) {
		csg_node_del(node);
		return NULL;
	} else {
		if (!csg_normalize_tree(node)) {
			csg_node_del(node);
			return NULL;
		} else {
			node = csg_prune_tree(node);
			return node;
		}
	}
}

static int feed_union_of_products(csg_union_of_products *uop, csg_node *node)
{
	/* this adds a product to a union of products */
	gltv_log_warning(GLTV_LOG_DEBUG, "feed node at %p\n", node);
	if (node->type==CSG_OP && node->u.op.op==CSG_UNION) {
		int ret1 = feed_union_of_products(uop, node->u.op.left);
		int ret2 = 0;
		if (ret1) ret2 = feed_union_of_products(uop, node->u.op.right);
		return ret1 && ret2;
	}
	/* here we are not in a union : we are then at the begening of a product.
	 * Lets go straight to the last left leaf, and write the whole product */
	/* in normal form, a non-union operator have only simple primitive on right side */
	{
		csg_node *node_tmp;
		unsigned nb = 1;
		for (node_tmp = node; node_tmp->type!=CSG_PRIM; node_tmp = node_tmp->u.op.left) {
			if (node_tmp->type!=CSG_OP || node_tmp->u.op.right->type!=CSG_PRIM) return 0;
			nb ++;
		}
		uop->unions[uop->nb_unions].nb_products = nb;
		nb--;
		for (node_tmp = node; node_tmp->type!=CSG_PRIM; node_tmp = node_tmp->u.op.left) {
			uop->unions[uop->nb_unions].products[nb].node = node_tmp->u.op.right;
			uop->unions[uop->nb_unions].products[nb-1].op_to_next = node_tmp->u.op.op;
			nb--;
		}
		uop->unions[uop->nb_unions].products[0].node = node_tmp;
		uop->nb_unions++;
		return 1;
	}
}

int csg_union_of_products_reset(csg_union_of_products *uop, csg_node *node)
{
	uop->nb_unions = 0;
	if (!feed_union_of_products(uop, node)) {
		return 0;
	} else {
		return 1;
	}
}

csg_union_of_products *csg_union_of_products_new(csg_node *node)
{
	/* given a pruned, normilized csg_tree, compute the flat union of products,
	 * or NULL if error */
	csg_union_of_products *csg_uop;
	csg_uop = gltv_memspool_alloc(sizeof(*csg_uop));
	if (!csg_uop) return NULL;
	if (!csg_union_of_products_reset(csg_uop, node)) {
		gltv_memspool_unregister(csg_uop);
		return NULL;
	}
	return csg_uop;
}

void csg_union_of_products_del(csg_union_of_products *uop)
{
	gltv_memspool_unregister(uop);
}

void csg_union_of_products_print(csg_union_of_products *uop)
{
	unsigned u, p;
	for (u=0; u<uop->nb_unions; u++) {
		if (u>0) putchar('|');
		printf(" ( ");
		for (p=0; p<uop->unions[u].nb_products; p++) {
			printf(" Prim(%s) ", uop->unions[u].products[p].node->u.prim.m->name);
			if (p<uop->unions[u].nb_products-1) putchar(operator[uop->unions[u].products[p].op_to_next]);
		}
		printf(" ) ");
	}
	putchar('\n');
}

int csg_union_of_partial_products_reset(csg_union_of_partial_products *uopp, csg_union_of_products *uop)
{
	unsigned uop_u;
	uopp->nb_unions = 0;
	for (uop_u=0; uop_u<uop->nb_unions; uop_u++) {
		/* faire autant d'unions partielles que de primitives dans l'union de produit */
		unsigned uop_p;
		for (uop_p=0; uop_p<uop->unions[uop_u].nb_products; uop_p++) {
			unsigned p, pp;
			/* chaque union partielle contient autant de primitives que l'union de produit (fo suivre) */
			uopp->unions[uopp->nb_unions].nb_products = uop->unions[uop_u].nb_products;
			uopp->unions[uopp->nb_unions].group_id = uop_u;
			uopp->unions[uopp->nb_unions].products[0].node = uop->unions[uop_u].products[uop_p].node;
			if (uop_p==0 || uop->unions[uop_u].products[uop_p-1].op_to_next==CSG_AND)
				uopp->unions[uopp->nb_unions].products[0].frontal = 1;	/* si uop_p==0 ou si opérateur précédent=and */
			else
				uopp->unions[uopp->nb_unions].products[0].frontal = 0;
			for (p=0, pp=1; p<uop->unions[uop_u].nb_products; p++) {
				if (p==uop_p) continue;
				uopp->unions[uopp->nb_unions].products[pp].node = uop->unions[uop_u].products[p].node;
				if (p==0 || uop->unions[uop_u].products[p-1].op_to_next==CSG_AND)
					uopp->unions[uopp->nb_unions].products[pp].frontal = 1;	/* si uop_p==0 ou si opérateur précédent=and */
				else
					uopp->unions[uopp->nb_unions].products[pp].frontal = 0;
				pp++;
			}
			uopp->nb_unions++;
		}
	}
	return 1;
}

csg_union_of_partial_products *csg_union_of_partial_products_new(csg_union_of_products *uop)
{
	csg_union_of_partial_products *csg_uopp;
	csg_uopp = gltv_memspool_alloc(sizeof(*csg_uopp));
	if (!csg_uopp) return NULL;
	if (!csg_union_of_partial_products_reset(csg_uopp, uop)) {
		gltv_memspool_unregister(csg_uopp);
		return NULL;
	} else {
		return csg_uopp;
	}
}

void csg_union_of_partial_products_del(csg_union_of_partial_products *uopp)
{
	gltv_memspool_unregister(uopp);
}
	
void csg_union_of_partial_products_resize(csg_union_of_partial_products *uopp)
{
	/* now, compute max2ds, max 2dbox amongst targets partials for each group */
	unsigned u=0, g=0;
	while (u<uopp->nb_unions) {	/* loop on g, group id  */
		b_box_2d box2d;
		/* compute boxes of the prims used in the group */
		if (u==0 || g!=uopp->unions[u-1].group_id) {
			unsigned p;
			for (p=0; p<uopp->unions[u].nb_products; p++) {
				geom_get_b_box(uopp->unions[u].products[p].node, &uopp->unions[u].products[p].node->u.prim.box3d);
				geom_get_b_box_2d(&uopp->unions[u].products[p].node->u.prim.box3d, &uopp->unions[u].products[p].node->u.prim.box2d);
			}
		}
		memcpy(&box2d, &uopp->unions[u].products[0].node->u.prim.box2d, sizeof(box2d));
		u++;
		while (u<uopp->nb_unions && uopp->unions[u].group_id==g) {
			if (box2d.vide) {
				memcpy(&box2d, &uopp->unions[u].products[0].node->u.prim.box2d, sizeof(box2d));
				u++;
				continue;
			}
			if (uopp->unions[u].products[0].node->u.prim.box2d.xmin<box2d.xmin) box2d.xmin=uopp->unions[u].products[0].node->u.prim.box2d.xmin;
			if (uopp->unions[u].products[0].node->u.prim.box2d.ymin<box2d.ymin) box2d.ymin=uopp->unions[u].products[0].node->u.prim.box2d.ymin;
			if (uopp->unions[u].products[0].node->u.prim.box2d.xmax>box2d.xmax) box2d.xmax=uopp->unions[u].products[0].node->u.prim.box2d.xmax;
			if (uopp->unions[u].products[0].node->u.prim.box2d.ymax>box2d.ymax) box2d.ymax=uopp->unions[u].products[0].node->u.prim.box2d.ymax;
			u++;
		}
		memcpy(&uopp->max2d_group[g], &box2d, sizeof(box2d));
		if (u<uopp->nb_unions) g=uopp->unions[u].group_id;
	}
}

void csg_union_of_partial_products_print(csg_union_of_partial_products *uopp)
{
	unsigned u, p;
	for (u=0; u<uopp->nb_unions; u++) {
		if (u>0) putchar('|');
		printf(" ( ");
		for (p=0; p<uopp->unions[u].nb_products; p++) {
			if (!uopp->unions[u].products[p].frontal) putchar('!');
			printf(" Prim(%s) ", uopp->unions[u].products[p].node->u.prim.m->name);
			if (p<uopp->unions[u].nb_products-1) putchar('&');
		}
		printf(" ) ");
	}
	putchar('\n');
}

static int uopp_identiqual(csg_union_of_partial_products *uopp_1, int g1, csg_union_of_partial_products *uopp_2, int g2)
{
	/* tells wether two uopps are the same */
	/* 2 uopps of a given group are the same if every subgroup are present in both.
	 * (at least, i think so :-p) */
	unsigned gg1, gg2;
	if (uopp_1->unions[g1].nb_products != uopp_2->unions[g2].nb_products) return 0;
	gg1 = g1;
	while (gg1<uopp_1->nb_unions && uopp_1->unions[gg1].group_id==uopp_1->unions[g1].group_id) {
		char ident = 0;
		gltv_log_warning(GLTV_LOG_DEBUG, "\tfor subgroup %u", gg1);
		gg2 = g2;
		while (gg2<uopp_2->nb_unions && uopp_2->unions[gg2].group_id==uopp_2->unions[g2].group_id) {
			unsigned p,pp;
			assert(uopp_1->unions[gg1].nb_products==uopp_2->unions[gg2].nb_products && uopp_1->unions[gg1].nb_products==uopp_1->unions[g1].nb_products);	/* every uopp of the same group should have the same nb_products ! */
			gltv_log_warning(GLTV_LOG_DEBUG, "\t...watch if subgroup of uopp2 %u matche", gg2);
			/* two unions of pp are equals if : same target and same "fromtality" of target,
			 * and same trimming prims/frontality, in any order */
			p = 0;
			if (uopp_1->unions[gg1].products[p].node->u.prim.m->prim==uopp_2->unions[gg2].products[p].node->u.prim.m->prim && uopp_1->unions[gg1].products[p].frontal==uopp_2->unions[gg2].products[p].frontal) {
				for (p=1; p<uopp_1->unions[g1].nb_products; p++) {
					for (pp=1; pp<uopp_1->unions[g1].nb_products; pp++) {
						gltv_log_warning(GLTV_LOG_DEBUG, "\t\tcomparing %s with %s and %d with %d",
								uopp_1->unions[gg1].products[p].node->u.prim.m->prim->name,uopp_2->unions[gg2].products[pp].node->u.prim.m->prim->name,
								(int)uopp_1->unions[gg1].products[p].frontal,(int)uopp_2->unions[gg2].products[pp].frontal);
						if (uopp_1->unions[gg1].products[p].node->u.prim.m->prim!=uopp_2->unions[gg2].products[pp].node->u.prim.m->prim || uopp_1->unions[gg1].products[p].frontal!=uopp_2->unions[gg2].products[pp].frontal)
							break;
					}
					if (pp==uopp_1->unions[g1].nb_products) {
						gltv_log_warning(GLTV_LOG_DEBUG, "\t\tfound");
						p = pp;
						break;
					}
				}
			} else {
				gltv_log_warning(GLTV_LOG_DEBUG, "\t\tno same targets");
			}
			if (p==uopp_1->unions[g1].nb_products) {
				ident = 1;
				gltv_log_warning(GLTV_LOG_DEBUG, "\tyes");
				break;
			} else gltv_log_warning(GLTV_LOG_DEBUG, "\tno - mismatch at %u",p);
			gg2++;
		}
		if (!ident) return 0;
		gg1++;
	}
	return 1;
}

int csg_is_equivalent(csg_union_of_partial_products *uopp_1, csg_union_of_partial_products *uopp_2)
{
	/* 2 csg are identiqual iff all groups are identiqual (ordering of groups is irrelevant) */
	unsigned g1, g2;
	if (uopp_1->nb_unions != uopp_2->nb_unions) return 0;
	g1 = 0;
	while (g1<uopp_1->nb_unions) {
		char ident = 0;
		gltv_log_warning(GLTV_LOG_DEBUG, "for group beginning at %u",g1);
		g2 = 0;
		while (g2<uopp_2->nb_unions) {
			gltv_log_warning(GLTV_LOG_DEBUG, "...watch if group in uopp2 beginning at %u matche", g2);
			if (uopp_identiqual(uopp_1, g1, uopp_2, g2)) {
				ident = 1;
				gltv_log_warning(GLTV_LOG_DEBUG, "yes");
				break;
			} else gltv_log_warning(GLTV_LOG_DEBUG, "no");
			/* look for next group in uopp_2 */
			do { g2++; } while (g2<uopp_2->nb_unions && uopp_2->unions[g2].group_id==uopp_2->unions[g2-1].group_id);
		}
		if (!ident) return 0;
		/* look for next group in uopp_1 */
		do { g1++; } while (g1<uopp_1->nb_unions && uopp_1->unions[g1].group_id==uopp_1->unions[g1-1].group_id);
	}
	return 1;
}

