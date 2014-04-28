/**
 * tb_clTree.h
 * Benchmark for OpenCL collaborative tree building - header
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
#ifndef CLTREE_TEST_H
#define CLTREE_TEST_H

#include "clQueue.h"
#include "clTree.h"

struct clTree_link {
	unsigned int source, sink;
};

struct clGraph_node {
	struct clTree_node tree;
	clqueue links;
};

struct clGraph_link {
	clqueue_item q;
	struct clGraph_node __global *sink;
};

#endif /* CLTREE_TEST_H_ */
