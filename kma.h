/**
 * kma.h
 * Superblock-based malloc
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
#ifndef KMA_H
#define KMA_H

#include "clheap.h"
#include "clIndexedQueue.h"

#define KMA_SB_SIZE 4096	/**< Superblock size: 4KB */
#define KMA_SB_SIZE_LOG2 12	/**< Log 2 of superblock size: 2^12=4K */
#define KMA_SB_SIZE_BUCKETS KMA_SB_SIZE_LOG2 - 1 /**< Size buckets */

#ifdef __OPENCL_CL_H
typedef volatile uintptr_t vg_uptr_t;

char *heapBack;

struct kma_heap_32 {
	uint32_t bytes;
	clIndexedQueue_32 free;			/**< Free list */
	uint32_t sb[KMA_SB_SIZE_BUCKETS];	/**< SB hashtbl*/
};

struct kma_heap_64 {
	uint64_t bytes;
	clIndexedQueue_64 free;			/**< Free list */
	uint64_t sb[KMA_SB_SIZE_BUCKETS];	/**< SB hashtbl*/
};

extern cl_mem kma_create(cl_device_id dev, cl_context ctx, cl_command_queue cq,
		cl_program prg, unsigned int);
int clheap_execute(cl_device_id, cl_context, cl_command_queue,cl_program,
		size_t);
#else
#if CL_BITNESS == 64
#pragma OPENCL EXTENSION cl_khr_int64_base_atomics : enable
#pragma OPENCL EXTENSION cl_khr_int64_extended_atomics : enable
#endif
#pragma OPENCL EXTENSION cl_khr_global_int32_base_atomics : enable
#pragma OPENCL EXTENSION cl_khr_global_int32_extended_atomics : enable
#pragma OPENCL EXTENSION cl_khr_local_int32_base_atomics : enable
#pragma OPENCL EXTENSION cl_khr_local_int32_extended_atomics : enable

/* A superblock consists of the following:
 * Header:		    state + next-pointer
 * Data:		    available size (SB size - header - bitfield)
 * free/allocated bitfield: padded to end, one bit for each block inside
 * 			    superblock
 *
 * Once full, it should be detached from the superblock hashtable. When all
 * blocks are free'd, the block should be returned to the free list.
 */
struct kma_sb {
	clIndexedQueue_item q;		/**< Link to the next queue item
					  !!!: Keep me on top!
					  or build a container_of()*/
	volatile unsigned int state; 	/**< Slots, slots free */
	unsigned int size;		/**< Size of a block */
	char data[KMA_SB_SIZE - 8 - sizeof(clIndexedQueue_item)];	/**< Rest of the header */
};

/* This heap administration will take up 1 superblock */
struct clheap {
	size_t bytes;
	clIndexedQueue free;				 	 /**< Free list */
	volatile struct kma_sb __global *sb[KMA_SB_SIZE_BUCKETS]; /**< SB hashtbl*/
};
#endif
#endif
