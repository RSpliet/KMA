/**
 * clTree.cl
 * A binary tree with unique nodes
 * Copyright (C) 2013-2014 Roy Spliet, Delft University of Technology
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301
 * USA
 */
#include "clTree.h"

/**
 * Do not ever change the key of a node after adding!
 * It could render the node unfindable..
 */
unsigned int
clTree_add(struct clTree __global *tree, struct clTree_node __global *node)
{
	volatile uintptr_t __global *cursor = (volatile uintptr_t __global *) &(tree->root);
	volatile struct clTree_node __global *cNow;
	uintptr_t cNode;

	node->left = NULL;
	node->right = NULL;
	mem_fence(CLK_GLOBAL_MEM_FENCE);;

	while(1) {
		cNode = atom_cmpxchg(cursor, NULL, (uintptr_t) node);
		mem_fence(CLK_GLOBAL_MEM_FENCE);
		if(cNode == NULL)
			return 1;
		/* else someone beat me to it! Carry on */

		cNow = (volatile struct clTree_node __global *) cNode;

		if(cNow->key == node->key)
			return 0;
		else if(node->key < cNow->key)
			cursor = (volatile uintptr_t __global *)&(cNow->left);
		else
			cursor = (volatile uintptr_t __global *)&(cNow->right);
	}

	return 0;
}

struct clTree_node __global *
clTree_get(struct clTree __global *tree, unsigned int key)
{
	volatile struct clTree_node __global *cursor = tree->root;

	while(cursor != NULL) {
		if(key == cursor->key)
			return cursor;

		if(key < cursor->key)
			cursor = cursor->left;
		else
			cursor = cursor->right;
	}

	return NULL;
}
