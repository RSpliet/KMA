/**
 * pma.cl
 * Main Poormans-heap OpenCL implementation
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
#define HEAP_H "pma.h"
#include HEAP_H

__kernel void
clheap_init(void __global *hp)
{
	struct clheap __global *heap = (struct clheap __global *)hp;
	heap->head = 0;
	heap->base = (char __global *) hp + sizeof(struct clheap);
}

void __global *
malloc(struct clheap __global *heap, size_t size)
{
	uint32_t ret, sz = size;

	ret = atom_add(&heap->head, sz);
	if((ret + size) > heap->bytes)
		return NULL;

	return (void __global *)&heap->base[ret];
}

void
free(struct clheap __global *heap, uintptr_t blk)
{
	return;
}
