/**
 * clPMMalloc.h
 * Poormans-heap allocator
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
#ifndef PMA_H
#define PMA_H

#include "clheap.h"

#ifdef __OPENCL_CL_H
#include <stdbool.h>
#include <stdint.h>

typedef volatile uintptr_t vg_uptr_t;

struct pma_heap_32 {
	uint32_t bytes;
	uint32_t base;
	uint32_t head;
};

struct pma_heap_64 {
	uint64_t bytes;
	uint64_t base;
	uint32_t head;
};

char *heapBack;
extern cl_mem pma_create(cl_device_id dev, cl_context ctx, cl_command_queue cq,
		cl_program prg, unsigned int);
#else
#define uint32_t unsigned int
#pragma OPENCL EXTENSION cl_khr_global_int32_base_atomics : enable
#pragma OPENCL EXTENSION cl_khr_global_int32_extended_atomics : enable

/* This heap administration will take up 1 superblock */
struct clheap {
	size_t bytes;
	char __global *base;
	volatile uint32_t head;
};

#endif
#endif

