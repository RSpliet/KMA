/**
 * clArrayList.h
 * Header definition for collaborative memory allocator w/ prefix-sum
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
#ifndef CLARRAYLIST_H
#define CLARRAYLIST_H
#include "clheap.h"
#include "clQueue.h"

#ifdef __OPENCL_CL_H
#include <stdbool.h>

/* silence IDE */
#define __global
#define __kernel
#define __constant

typedef struct {
	uint32_t objSize;
	clqueue_32 queue;
	uint32_t heap;
	uint32_t reduce_mem;
} clArrayList_32;

typedef struct {
	uint64_t objSize;
	clqueue_64 queue;
	uint64_t heap;
	uint64_t reduce_mem;
} clArrayList_64;

extern cl_mem clArrayList_create(cl_device_id dev, cl_context ctx,
	cl_command_queue cq, cl_program prg, cl_uint objsize, cl_mem heap);
#else
typedef struct {
	size_t objSize;
	clqueue queue;
	struct clheap __global *heap;
	uintptr_t __global *reduce_mem;
} clArrayList;

struct clArrayList_page {
	clqueue_item next;
	size_t count;
	size_t taken;
};

uintptr_t clArrayList_grow_local(clArrayList __global *, unsigned int,
		uintptr_t __local *);
#endif
#endif
