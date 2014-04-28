/**
 * clTree.h
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
#ifndef CLTREE_H
#define CLTREE_H

struct clTree_node{
	unsigned int key;
	volatile struct clTree_node __global *left;
	volatile struct clTree_node __global *right;
};

struct clTree {
	volatile struct clTree_node __global *root;
};

unsigned int clTree_add(struct clTree __global *, struct clTree_node __global *);
struct clTree_node __global *clTree_get(struct clTree __global *, unsigned int);

#endif /* CLTREE_H */
